#include <eosio/eosio.hpp>
#include <eosio/system.hpp>
#include <eosio/binary_extension.hpp>
#include <eosio/producer_schedule.hpp>

#include <string>

using namespace eosio;

class [[eosio::contract("eosio")]] system_contract : public contract
{
public:
    using contract::contract;

    [[eosio::action]]
    void setproducer( const name owner, const double total_votes, const bool is_active )
    {
        // set producer
        producers_table _producers(get_self(), get_self().value);
        _producers.emplace(get_self(), [&](auto& p) {
            p.owner = owner;
            p.total_votes = total_votes;
            p.is_active = is_active;
        });
    }

    static eosio::block_signing_authority convert_to_block_signing_authority( const eosio::public_key& producer_key ) {
        return eosio::block_signing_authority_v0{ .threshold = 1, .keys = {{producer_key, 1}} };
    }

    // Defines `producer_info` structure to be stored in `producer_info` table, added after version 1.0
    struct [[eosio::table, eosio::contract("eosio.system")]] producer_info {
        name                                                     owner;
        double                                                   total_votes = 0;
        eosio::public_key                                        producer_key; /// a packed public key object
        bool                                                     is_active = true;
        std::string                                              url;
        uint32_t                                                 unpaid_blocks = 0;
        time_point                                               last_claim_time;
        uint16_t                                                 location = 0;
        eosio::binary_extension<eosio::block_signing_authority>  producer_authority; // added in version 1.9.0

        uint64_t primary_key()const { return owner.value;                             }
        double   by_votes()const    { return is_active ? -total_votes : total_votes;  }
        bool     active()const      { return is_active;                               }
        void     deactivate()       { producer_key = public_key(); producer_authority.reset(); is_active = false; }

        eosio::block_signing_authority get_producer_authority()const {
            if( producer_authority.has_value() ) {
                bool zero_threshold = std::visit( [](auto&& auth ) -> bool {
                    return (auth.threshold == 0);
                }, *producer_authority );
                // zero_threshold could be true despite the validation done in regproducer2 because the v1.9.0 eosio.system
                // contract has a bug which may have modified the producer table such that the producer_authority field
                // contains a default constructed eosio::block_signing_authority (which has a 0 threshold and so is invalid).
                if( !zero_threshold ) return *producer_authority;
            }
            return convert_to_block_signing_authority( producer_key );
        }

        // The unregprod and claimrewards actions modify unrelated fields of the producers table and under the default
        // serialization behavior they would increase the size of the serialized table if the producer_authority field
        // was not already present. This is acceptable (though not necessarily desired) because those two actions require
        // the authority of the producer who pays for the table rows.
        // However, the rmvproducer action and the onblock transaction would also modify the producer table in a similar
        // way and increasing its serialized size is not acceptable in that context.
        // So, a custom serialization is defined to handle the binary_extension producer_authority
        // field in the desired way. (Note: v1.9.0 did not have this custom serialization behavior.)

        template<typename DataStream>
        friend DataStream& operator << ( DataStream& ds, const producer_info& t ) {
            ds << t.owner
                << t.total_votes
                << t.producer_key
                << t.is_active
                << t.url
                << t.unpaid_blocks
                << t.last_claim_time
                << t.location;

            if( !t.producer_authority.has_value() ) return ds;

            return ds << t.producer_authority;
        }

        template<typename DataStream>
        friend DataStream& operator >> ( DataStream& ds, producer_info& t ) {
            return ds >> t.owner
                         >> t.total_votes
                         >> t.producer_key
                         >> t.is_active
                         >> t.url
                         >> t.unpaid_blocks
                         >> t.last_claim_time
                         >> t.location
                         >> t.producer_authority;
        }
    };

    typedef eosio::multi_index< "producers"_n, producer_info,
        indexed_by<"prototalvote"_n, const_mem_fun<producer_info, double, &producer_info::by_votes>>
    > producers_table;
};