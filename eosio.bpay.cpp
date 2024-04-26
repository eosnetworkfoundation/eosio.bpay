#include "eosio.bpay.hpp"

namespace eosio {

[[eosio::action]]
void bpay::claimrewards( const name owner ) {
    require_auth( owner );

    rewards_table _rewards( get_self(), get_self().value );

    const auto& row = _rewards.get( owner.value, "not eligible to claim producer block pay" );
    check( row.quantity.amount > 0, "no rewards to claim");

    // transfer rewards to owner
    eosio::token::transfer_action transfer( "eosio.token"_n, { get_self(), "active"_n });
    transfer.send( get_self(), owner, row.quantity, "producer block pay" );

    _rewards.modify(row, get_self(), [&](auto& row) {
        row.quantity.amount = 0;
    });
}

[[eosio::on_notify("*::transfer")]]
void bpay::on_transfer( const name from, const name to, const asset quantity, const string memo )
{
    if (from == get_self() || to != get_self()) {
        return;
    }

    check( get_first_receiver() == "eosio.token"_n, "only eosio.token allowed") ;
    check( quantity.symbol == eosio::symbol("EOS", 4), "only EOS token allowed" );

    rewards_table _rewards( get_self(), get_self().value );
    eosiosystem::system_contract::producers_table _producers( "eosio"_n, "eosio"_n.value );

    // calculate rewards equal share for top 21 producers
    asset reward = quantity / 21;

    // get producer with the most votes
    // using `by_votes` secondary index
    auto idx = _producers.get_index<"prototalvote"_n>();
    auto prod = idx.begin();

    // get top 21 producers by vote
    std::vector<name> top_producers;
    while (true) {
        if ( prod == idx.end() ) {
            break;
        }
        if ( prod->is_active == false ) {
            continue;
        }
        top_producers.push_back(prod->owner);
        print("rank=", top_producers.size(), " producer=", prod->owner, " reward=", reward.to_string(), "\n");
        if ( top_producers.size() == 21 ) {
            break;
        }
        prod++;
    }

    // update rewards table
    for (auto producer : top_producers) {
        auto row = _rewards.find( producer.value );
        if (row == _rewards.end()) {
            _rewards.emplace( get_self(), [&](auto& row) {
                row.owner = producer;
                row.quantity = reward;
            });
        } else {
            _rewards.modify(row, get_self(), [&](auto& row) {
                row.quantity += reward;
            });
        }
    }
}

} /// namespace eosio
