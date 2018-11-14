namespace webgame {

template<class EntityType>
entity_container<EntityType>::entity_container(std::initializer_list<std::shared_ptr<EntityType>> const& ilist)
{
    for (std::shared_ptr<entity> const& ent_p : ilist)
        add(ent_p);
}

template<class EntityType>
std::shared_ptr<EntityType> const& entity_container<EntityType>::add(std::shared_ptr<EntityType> const& ent_ptr)
{
    auto ret = this->insert(std::make_pair(ent_ptr->id(), ent_ptr));
    assert(ret.second);
    return ret.first->second;
}

template<class EntityType>
template<class SubType>
entity_container<EntityType>::operator entity_container<SubType>()
{
    entity_container<SubType> sub_container;
    for (auto const& pair : *this)
    {
        std::shared_ptr<SubType> true_ptr = std::dynamic_pointer_cast<SubType>(pair.second);
        if (true_ptr)
            sub_container.add(true_ptr);
    }
    return sub_container;
}

} // namespace webgame
