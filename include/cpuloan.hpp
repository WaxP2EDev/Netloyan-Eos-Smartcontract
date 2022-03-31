#pragma once
#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/transaction.hpp>
#include <token.hpp>

#include <stdint.h>
#include <string>

using namespace eosio;
using namespace std;

namespace eosiosystem {
   class system_contract;
}

namespace eosio {

   using std::string;

   struct pair {
      uint32_t days;
      double fees;
   };

   class [[eosio::contract("cpuloan")]] cpuloan : public contract {
      public:
         using contract::contract;
        static constexpr name TOKEN_CONTRACT = "eosio.token"_n;
        static constexpr symbol WAX_SYMBOL = symbol(symbol_code("WAX"), 8);

         [[eosio::action]]
         void logstake(const name account,const time_point_sec unstakeTime,const asset stakedcpu);

         [[eosio::action]]
         void logunstake(const asset unstaked,const name account);

         [[eosio::action]]
         void process();

         [[eosio::on_notify("eosio.token::transfer")]] void stakecheck(const name &from, const name &to, const asset &quantity, const string &memo);

         [[eosio::action]]
         void testreset();

         [[eosio::action]]
         void setconfig(const uint32_t unstakeSeconds,const asset min_amount ,const vector<pair> cpu_multiplier);

         [[eosio::action]]
         void admintransfr();

         [[eosio::action]]
         void waxrefund();  
         
         void inlineunstak(const uint64_t id);

         [[eosio::action]]
         void adminunstake(const uint64_t id);
      private:

         struct [[eosio::table]] config_data {
            uint64_t id;
            vector<pair> cpu_multiplier;
            asset min_amount;
            uint32_t unstakeSeconds;
            auto primary_key()const { return id;};
         };

         typedef eosio::multi_index< "config"_n, config_data > config_s;

                  struct [[eosio::table]] staking_data {
            uint64_t id;
            name account;
            time_point_sec unstakeTime;  //YY-MM-DDTH:M:S (Time format to follow)
            asset stakedcpu;
            auto primary_key()const { return id;};

         };

         typedef eosio::multi_index< "staking"_n, staking_data > staking_s;

         
    staking_s staking = staking_s(get_self(), get_self().value); 
    config_s config = config_s(get_self(), get_self().value); 

    int rfinder(vector<pair> cpu_multipliers, int day_count);

   };

}