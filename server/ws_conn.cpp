#include "ws_conn.hpp"

#include <boost/asio/bind_executor.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#include "misc/json.hpp"
#include "misc/log.hpp"
#include "misc/utils.hpp"
#include "misc/vector.hpp"

class entity;

std::vector<std::string> const ws_conn::state_str = {
    "none",
    "ready",
    "handshaking",
    "reading",
    "writing",
    "to_be_closed",
    "closing",
    "closed"
};

#define CONN_LOG(to_log) LOG(addr_str, to_log)

ws_conn::ws_conn(asio::ip::tcp::socket &socket, std::shared_ptr<entity const> entity)
    : addr_str(socket.remote_endpoint().address().to_string() + ":" + std::to_string(socket.remote_endpoint().port()))
    , socket_(std::move(socket))
    , strand_(socket_.get_executor())
    , state_(none)
    , player_entity_(entity)
{
    state_ = ready;
    socket_.auto_fragment(true);

    socket_.control_callback([&](beast::websocket::frame_type f, beast::string_view s) {
        CONN_LOG("CONTROL FRAME: " << s.to_string());
    });

    CONN_LOG("CONNECTED");
}

void ws_conn::start()
{
    std::lock_guard<decltype(handlers_mutex_)> l(handlers_mutex_);

    CONN_LOG("HANDSHAKING");

    socket_.async_accept(asio::bind_executor(strand_, std::bind(&ws_conn::on_accept, shared_from_this(), std::placeholders::_1)));
    state_ = handshaking;
}

void ws_conn::write(std::shared_ptr<std::string const> msg)
{
    std::lock_guard<decltype(handlers_mutex_)> l(handlers_mutex_);

    to_write_.emplace_back(msg);

    if (to_write_.size() == 1)
        asio::post(socket_.get_executor(), asio::bind_executor(strand_, std::bind(&ws_conn::write_next, shared_from_this())));
}

void ws_conn::close()
{
    do_close(beast::websocket::close_code::normal);
}

bool ws_conn::pop_patch(patch &out)
{
    std::lock_guard<decltype(patches_mutex_)> l(patches_mutex_);

    if (patches_.empty())
        return false;
    out = patches_.front();
    patches_.pop();
    return true;
}

ws_conn::socket_t& ws_conn::socket()
{
    return socket_;
}

bool ws_conn::is_closed() const
{
    return state_ == closed;
}

std::shared_ptr<entity const> ws_conn::player_entity() const
{
    return player_entity_;
}

ws_conn::state ws_conn::current_state() const
{
    return state_;
}

void ws_conn::write_next()
{
    std::lock_guard<decltype(handlers_mutex_)> l(handlers_mutex_);

    if (state_ != reading || to_write_.empty())
        return;

    socket_.async_write(asio::buffer(*to_write_.back()), asio::bind_executor(strand_, std::bind(&ws_conn::on_write, shared_from_this(), std::placeholders::_1, std::placeholders::_2)));
    state_ = writing;
}

void ws_conn::on_accept(boost::system::error_code const& ec) noexcept
{
    std::lock_guard<decltype(handlers_mutex_)> l(handlers_mutex_);

    if (ec)
    {
        CONN_LOG("HANDSHAKING ERROR" << ": " << ec.message());
        do_close(beast::websocket::close_code::abnormal);
        do_read();
        return;
    }

    CONN_LOG("HANDSHAKING DONE");

    do_read();
    state_ = reading;

    boost::property_tree::ptree root = entity_to_ptree(player_entity_);
    root.put("order", "state");
    root.put("suborder", "player");
    std::stringstream ss;
    boost::property_tree::write_json(ss, root, false);
    write(std::make_shared<std::string const>(std::move(ss.str())));
}

void ws_conn::on_read(boost::system::error_code const& ec, std::size_t const& bytes_transferred) noexcept
{
    std::lock_guard<decltype(handlers_mutex_)> l(handlers_mutex_);

    assert(state_ != closed);

    if (ec)
    {
        assert(read_buffer_.size() == 0);

        if (socket_.is_open())
        {
            CONN_LOG("READ ERROR: " << ec.message() << ", code: " << ec.value());
            do_close(beast::websocket::close_code::normal);
            do_read();
        }
        else
        {
            CONN_LOG("READ: SESSION CLOSED");
            state_ = closed;
        }

        return;
    }
    else if (state_ == closing)
    {
        CONN_LOG("READ: Cannot read: Socket is closing, " << bytes_transferred << " bytes");
        read_buffer_.consume(read_buffer_.size());
        return;
    }

    if (io_log && data_log)
        CONN_LOG("READ " << bytes_transferred << " BYTES: " << std::endl << get_readable(read_buffer_.data()));
    else if (io_log)
        CONN_LOG("READ " << bytes_transferred << " BYTES");

    auto str = boost::beast::buffers_to_string(read_buffer_.data());
    read_buffer_.consume(read_buffer_.size());

    std::istringstream iss(str);
    boost::property_tree::ptree ptree;

    try {
        boost::property_tree::read_json(iss, ptree);

        std::string order = ptree.get<std::string>("order");
        if (order == "state")
        {
            std::string suborder = ptree.get<std::string>("suborder");
            if (suborder == "player")
            {
                std::lock_guard<decltype(patches_mutex_)> l(patches_mutex_);
                if (ptree.find("speed") != ptree.not_found())
                    push_patch("speed", any(ptree.get<real>("speed")));

                if (ptree.find("vel") != ptree.not_found() && ptree.find("vel") != ptree.not_found())
                    push_patch("vel", any(vector({ ptree.get<real>("vel.x"), ptree.get<real>("vel.y") })));

                if (ptree.find("targetPos") != ptree.not_found() && ptree.find("targetPos") != ptree.not_found())
                    push_patch("targetPos", any(vector({ ptree.get<real>("targetPos.x"), ptree.get<real>("targetPos.y") })));
            }
        }
    }
    catch (boost::property_tree::json_parser_error const& e) {
        CONN_LOG("READ: JSON PARSER ERROR: " << e.what());
    }
    catch (...) {
        CONN_LOG("READ: EXCEPTION THROWN");
    }

    do_read();
}

void ws_conn::on_write(beast::error_code const& ec, std::size_t const& bytes_transferred) noexcept
{
    std::lock_guard<decltype(handlers_mutex_)> l(handlers_mutex_);

    auto msg = to_write_.back();
    to_write_.pop_back();

    if (!ec && bytes_transferred != msg->size())
        CONN_LOG("ON WRITE: >>> no error, but bytes_transferred != expected: " << bytes_transferred << " != " << msg->size());

    if (state_ == to_be_closed)
    {
        CONN_LOG("ON WRITE: Session should be closed, closing it");
        do_close(beast::websocket::close_code::normal);
        return;
    }

    if (state_ != writing)
        return;

    state_ = reading;

    if (ec)
    {
        if (ec.value() == 995)
            CONN_LOG("WRITE INFO: operation aborted");
        else
            CONN_LOG("WRITE ERROR: " << ec.message());
        return;
    }

    if (io_log && data_log)
        CONN_LOG("ON WRITE: " << bytes_transferred << " WRITTEN: " << std::endl << *msg);
    else if (io_log)
        CONN_LOG("ON WRITE: " << bytes_transferred << " WRITTEN");

    write_next();
}

void ws_conn::on_close() noexcept
{
    CONN_LOG("ON CLOSE: Reading until error");
}

void ws_conn::do_read()
{
    socket_.async_read(read_buffer_, asio::bind_executor(strand_, std::bind(&ws_conn::on_read, shared_from_this(), std::placeholders::_1, std::placeholders::_2)));
}

void ws_conn::do_close(beast::websocket::close_code const& code)
{
    std::lock_guard<decltype(handlers_mutex_)> l(handlers_mutex_);

    assert(state_ != closed);

    if (state_ == writing)
    {
        CONN_LOG("FORCED CLOSE: Cannot close socket yet, there is a pending write");
        state_ = to_be_closed;
        return;
    }

    CONN_LOG("CLOSING");

    if (!socket_.is_open())
        CONN_LOG("ERROR: socket_.is_open() -> " << socket_.is_open());

    if (socket_.get_executor().running_in_this_thread())
        socket_.async_close(code, asio::bind_executor(strand_, std::bind(&ws_conn::on_close, shared_from_this())));
    else
        asio::post(socket_.get_executor(), asio::bind_executor(strand_, std::bind(&ws_conn::do_close, shared_from_this(), code)));
    state_ = closing;
}

void ws_conn::push_patch(std::string const& what, any const& value)
{
    std::lock_guard<decltype(patches_mutex_)> l(patches_mutex_);

    patch p;
    p.what = what;
    p.value = value;
    push_patch(p);
}

void ws_conn::push_patch(patch const& p)
{
    std::lock_guard<decltype(patches_mutex_)> l(patches_mutex_);

    patches_.push(p);
}
