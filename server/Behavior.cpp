#include "Behavior.hpp"

Walk::Walk()
    :t_(0)
{}

void Walk::update(Entity &self, std::map<int, Entity> &others, float delta)
{
    t_ += delta;
    if (self.pos()[0] > 1 || self.pos()[0] < -1 || self.pos()[1] > 1 || self.pos()[1] < -1)
        self.setVel(self.vel() * -1);
    if (t_ >= 1.)
    {
        auto dir = ::rand() % 8;
        if (dir == 0)
            self.setVel(Array2({ 1., 0. }));
        else if (dir == 1)
            self.setVel(Array2({ 1., 1. }));
        else if (dir == 2)
            self.setVel(Array2({ 0., 1. }));
        else if (dir == 3)
            self.setVel(Array2({ -1., 1. }));
        else if (dir == 4)
            self.setVel(Array2({ -1., 0. }));
        else if (dir == 5)
            self.setVel(Array2({ -1., -1. }));
        else if (dir == 6)
            self.setVel(Array2({ 0., -1. }));
        else if (dir == 7)
            self.setVel(Array2({ 1., -1. }));

        self.setVel(self.vel() * -1);
        self.setVel(VecType(self.vel() / boost::numeric::ublas::norm_2(self.vel())));
        t_ = 0.;
    }
}
