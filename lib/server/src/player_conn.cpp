#include "player_conn.hpp"

#include <boost/asio/bind_executor.hpp>
#include <boost/beast/core/buffers_to_string.hpp>

#include <nlohmann/json.hpp>

#include "any.hpp"
#include "lock.hpp"
#include "log.hpp"
#include "player.hpp"
#include "protocol.hpp"
#include "server.hpp"
#include "utils.hpp"
#include "vector.hpp"

namespace asio = boost::asio;
namespace beast = boost::beast;

namespace webgame {

class entity;

std::vector<std::string> const player_conn::state_str = {
    "none",
    "ready",
    "handshaking",
    "authenticating",
    "loading_player",
    "reading",
    "writing",
    "to_be_closed",
    "closing",
    "closed"
};

#define CONN_LOG(to_log) WEBGAME_LOG(addr_str, to_log)

player_conn::patch::patch(std::string &&w, any &&v)
    : what(std::move(w))
    , value(std::move(v))
{}

player_conn::player_conn(asio::ip::tcp::socket &&socket, boost::asio::io_context &ioc, std::shared_ptr<server> const& server)
    : addr_str(socket.remote_endpoint().address().to_string() + ":" + std::to_string(socket.remote_endpoint().port()))
    , socket_(std::move(socket))
    , strand_(ioc)
    , state_(none)
    , close_code_(beast::websocket::close_code::none)
    , server_(server)
    , close_timer_(socket_.get_executor())
{
    state_ = ready;
    socket_.auto_fragment(true);

    CONN_LOG("CONNECTED");
}

void player_conn::start()
{
    WEBGAME_LOCK(handlers_mutex_);

    socket_.async_accept(asio::bind_executor(strand_, std::bind(&player_conn::on_accept, shared_from_this(), std::placeholders::_1)));
    state_ = handshaking;
}

void player_conn::write(std::shared_ptr<std::string const> msg)
{
    WEBGAME_LOCK(handlers_mutex_);

    to_write_.emplace_back(msg);

    if (to_write_.size() == 1)
        asio::post(socket_.get_executor(), asio::bind_executor(strand_, std::bind(&player_conn::write_next, shared_from_this())));
}

void player_conn::close()
{
    if (strand_.running_in_this_thread())
        do_close(beast::websocket::close_code::normal);
    else
        asio::post(socket_.get_executor(), asio::bind_executor(strand_, std::bind(&player_conn::do_close, shared_from_this(), beast::websocket::close_code::normal)));
    state_ = state::closed;
}

player_conn::patch player_conn::pop_patch()
{
    WEBGAME_LOCK(patches_mutex_);

    patch p = std::move(patches_.front());
    patches_.pop();
    return p;
}

bool player_conn::has_patch() const
{
    WEBGAME_LOCK(patches_mutex_);

    return !patches_.empty();
}

bool player_conn::is_closed() const
{
    return state_ == closed;
}

std::shared_ptr<player> const& player_conn::player_entity() const
{
    return player_entity_;
}

player_conn::state player_conn::current_state() const
{
    return state_;
}

bool player_conn::is_ready() const
{
    return state_ == reading || state_ == writing;
}

std::string const& player_conn::player_name() const
{
    return player_name_;
}

void player_conn::write_next()
{
    WEBGAME_LOCK(handlers_mutex_);

    if (!(state_ == loading_player || state_ == reading) || to_write_.empty())
        return;

    socket_.async_write(asio::buffer(*to_write_.front()), asio::bind_executor(strand_, std::bind(&player_conn::on_write, shared_from_this(), std::placeholders::_1, std::placeholders::_2)));
    state_ = writing;
}

void player_conn::on_accept(boost::system::error_code const& ec) noexcept
{
    WEBGAME_LOCK(handlers_mutex_);

    if (ec)
    {
        CONN_LOG("HANDSHAKE ERROR" << ": " << ec.message());
        do_close(beast::websocket::close_code::abnormal);
        do_read();
        return;
    }

    CONN_LOG("HANDSHAKED");

    do_read();
    state_ = authenticating;
}

void player_conn::on_read(boost::system::error_code const& ec, std::size_t const& bytes_transferred) noexcept
{
    WEBGAME_LOCK(handlers_mutex_);

    assert(state_ != closed);

    if (ec)
    {
        if (socket_.is_open())
        {
            CONN_LOG("READ ERROR: " << ec.message() << ", code: " << ec.value());
            do_close(beast::websocket::close_code::abnormal);
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
    else if (state_ == loading_player)
    {
        CONN_LOG("READ: ERROR: NOT SUPPOSED TO RECEIVE DATA WHILE LOADING PLAYER ENTITY");
        do_close(beast::websocket::close_code::abnormal);
        do_read();
        return;
    }

    if (io_log && data_log)
        CONN_LOG("READ " << bytes_transferred << " BYTES: " << std::endl << get_readable(read_buffer_.data()));
    else if (io_log)
        CONN_LOG("READ " << bytes_transferred << " BYTES");

    std::string order_str = boost::beast::buffers_to_string(read_buffer_.data());
    read_buffer_.consume(read_buffer_.size());

    interpret(std::move(order_str));

    do_read();
}

void player_conn::on_write(beast::error_code const& ec, std::size_t const& bytes_transferred) noexcept
{
    WEBGAME_LOCK(handlers_mutex_);

    auto msg = to_write_.front();
    to_write_.pop_front();

    if (ec)
    {
        if (ec.value() == 995)
            CONN_LOG("WRITE ERROR: operation aborted");
        else
            CONN_LOG("WRITE ERROR: " << ec.message());
        return;
    }

    if (state_ == to_be_closed)
    {
        CONN_LOG("ON WRITE: Session should be closed, closing it");
        do_close(beast::websocket::close_code::normal);
        return;
    }

    if (state_ != writing)
        return;

    state_ = reading;

    if (io_log && data_log)
        CONN_LOG("ON WRITE: " << bytes_transferred << " WRITTEN: " << std::endl << *msg);
    else if (io_log)
        CONN_LOG("ON WRITE: " << bytes_transferred << " WRITTEN");

    write_next();
}

void player_conn::on_close() noexcept
{
    WEBGAME_LOCK(handlers_mutex_);
    if (close_timer_.cancel() == 1)
        CONN_LOG("ON CLOSE: GRACEFUL CLOSE");
    else
        CONN_LOG("ON CLOSE: FORCED CLOSE");
}

void player_conn::on_player_load(std::shared_ptr<player> const& player_entity)
{
    if (state_ != loading_player)
        return;

    CONN_LOG("PLAYER LOADED id=" << player_entity->id());

    player_entity_ = player_entity;
    player_entity->set_conn(this);

    server_->register_player(shared_from_this(), player_entity);

    state_ = reading;
}

void player_conn::do_read()
{
    socket_.async_read(read_buffer_, asio::bind_executor(strand_, std::bind(&player_conn::on_read, shared_from_this(), std::placeholders::_1, std::placeholders::_2)));
}

void player_conn::do_close(beast::websocket::close_code const& code)
{
    WEBGAME_LOCK(handlers_mutex_);

    if (state_ == closed || state_ == closing)
    {
        CONN_LOG("WARNING: TRYING TO CLOSE BUT SOCKET IS ALREADY IN " << (state_ == closed ? "CLOSED" : "CLOSING") << " STATE");
        return;
    }

    if (state_ == writing)
    {
        CONN_LOG("CLOSE: Cannot close socket yet, there is a pending write");
        state_ = to_be_closed;
        close_code_ = code;
        return;
    }

    CONN_LOG("CLOSING");

    // If the endpoint does not acknowledge the close after N seconds, we force it by closing the underlying tcp socket
    close_timer_.expires_after(std::chrono::seconds(10));
    std::shared_ptr<player_conn> this_p = shared_from_this();
    close_timer_.async_wait([this, this_p](boost::system::error_code const& error) {
        WEBGAME_LOCK(handlers_mutex_);
        if (state_ != closed && socket_.next_layer().is_open())
        {
            CONN_LOG("WEBSOCKET CLOSE DID NOT SUCCEED. CLOSING THE TCP SOCKET");
            socket_.next_layer().shutdown(asio::ip::tcp::socket::shutdown_both);
            socket_.next_layer().close();
        }
    });

    socket_.async_close(code, asio::bind_executor(strand_, std::bind(&player_conn::on_close, shared_from_this())));
    state_ = closing;
}

void player_conn::interpret(std::string &&order_str)
{
    //CONN_LOG("INTERPRETING " << order_str);

    try {
        nlohmann::json j = nlohmann::json::parse(std::move(order_str));

        std::string order = j["order"];
        if (state_ == authenticating)
        {
            if (order != "authentication")
                throw std::runtime_error("AUTHENTICATION: NOT AN AUTHENTICATION ORDER");

            std::string player_name = j["player_name"];
            if (server_->is_player_connected(player_name))
                throw std::runtime_error("AUTHENTICATION: PLAYER " + player_name + " ALREADY CONNECTED");

            if (player_name.empty())
                throw std::runtime_error("AUTHENTICATION: INVALID PLAYER NAME");

            player_name_ = player_name;

            CONN_LOG("LOADING PLAYER " << player_name_);
            server_->get_persistence()->async_load_player(player_name_, std::bind(&player_conn::on_player_load, shared_from_this(), std::placeholders::_1));
            state_ = loading_player;
        }
        else if (order == "action")
        {
            std::string suborder = j["suborder"];
            WEBGAME_LOCK(patches_mutex_);

            if (suborder == "change_speed")
                push_patch("speed", any(j["speed"].get<double>()));
            else if (suborder == "change_dir")
                push_patch("dir", any(vector({ j["dir"]["x"].get<double>(), j["dir"]["y"].get<double>() })));
            else if (suborder == "move_to")
                push_patch("target_pos", any(vector({ j["target_pos"]["x"].get<double>(), j["target_pos"]["y"].get<double>() })));
            else
                throw std::runtime_error("UNKNOWN ACTION: " + suborder);
        }
        else
            throw std::runtime_error("UNKNOWN ORDER: " + order);
    }
    catch (std::exception const& e) {
        CONN_LOG("INTERPRET ERROR: " << e.what());
        do_close(beast::websocket::close_code::abnormal);
    }
}

void player_conn::push_patch(std::string && what, any && value)
{
    WEBGAME_LOCK(patches_mutex_);

    push_patch(patch(std::move(what), std::move(value)));
}

void player_conn::push_patch(patch && p)
{
    WEBGAME_LOCK(patches_mutex_);

    patches_.emplace(std::move(p));
}

} // namespace webgame
