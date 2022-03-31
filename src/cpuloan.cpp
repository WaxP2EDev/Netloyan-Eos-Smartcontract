#include <cpuloan.hpp>
#include <eosio/system.hpp>
#include <eosio/time.hpp>
#include <stdint.h>
#include <math.h>


namespace eosio {

using eosio::current_time_point;
using eosio::time_point_sec;

static inline time_point_sec current_time_point_sec() {
  return time_point_sec(current_time_point());
}

void cpuloan::stakecheck(const name &from, const name &to,
                         const asset &quantity, const string &memo) {

  if (from == get_self()||from==name("eosio.stake")) {
    return;
  }

   if(memo!=""){
  
    size_t start;
    size_t end = 0;
    char delim = '%';
    std::vector<string> out;

    while ((start = memo.find_first_not_of(delim, end)) != string::npos) {
      end = memo.find(delim, start);
      out.push_back(memo.substr(start, end - start));
    }
    

  check(to == get_self(), "contract is not involved in this transfer");
  check(quantity.symbol.is_valid(), "invalid quantity");
  check(quantity.amount > 0, "only positive quantity allowed");
  check(quantity.symbol == WAX_SYMBOL, "only WAX tokens allowed");
    

    uint32_t id = (uint32_t)stoull(out[0]);
    
    string account= out[1];

    name receiver_account=out.size()>1?name(account):from;  

    int days_ = std::stoi(memo);
    uint32_t days = (uint32_t)days_;

    auto config_ = config.begin();

    check(days >= 1,"Minimum time limit is 1 day");
    check(days <= 30,"Day limit exceeded");
    check(days == 1 || days%5 == 0,"Invalid day input");
    check(quantity.amount >= config_->min_amount.amount,"Amount is lesser than minimum allowed!");
    int x =  rfinder(config_->cpu_multiplier,days);
    check(x != -1, "No fees rate found");
    float fee = (float)config_->cpu_multiplier[x].fees / 100;
    uint64_t stakingamount = ((float)quantity.amount / fee);
    asset stake_cpu_asset;
    stake_cpu_asset.symbol = WAX_SYMBOL;
    stake_cpu_asset.amount = stakingamount;
    asset stake_net_asset;
    stake_net_asset.symbol = WAX_SYMBOL;
    stake_net_asset.amount = 0;
    bool transfer = false;
    time_point_sec unstakeTime;
    unstakeTime = current_time_point_sec() + config_->unstakeSeconds*days;

    action(permission_level{get_self(), "active"_n}, "eosio"_n, "delegatebw"_n,
           std::make_tuple(get_self(), receiver_account, stake_net_asset, stake_cpu_asset,
                           false))
        .send();

    action(permission_level{get_self(), "active"_n}, get_self(),
           "logstake"_n, std::make_tuple(receiver_account, unstakeTime, stake_cpu_asset))
        .send();
   }
}

void cpuloan::logstake(const name account, const time_point_sec unstakeTime,
                       const asset stakedcpu) {
  require_auth(get_self());
 

  staking.emplace(get_self(), [&](auto &v) {
    v.id=staking.available_primary_key();
    v.account = account;
    v.stakedcpu.symbol = stakedcpu.symbol;
    v.stakedcpu.amount = stakedcpu.amount;
    v.unstakeTime = unstakeTime;
  });


}

void cpuloan::process() {
  require_auth(get_self());
  auto stakes = staking.begin();
  while (stakes != staking.end()) {
    uint64_t id=-1;
    if (current_time_point_sec() >= stakes->unstakeTime) {
      id=stakes->id;
    } 
      stakes++;
      if(id!=-1)
      inlineunstak(id);     
  }
}

void cpuloan::inlineunstak(const uint64_t id) {
  auto stakes = staking.find(id);
  asset stake_net_asset;
  stake_net_asset.symbol = WAX_SYMBOL;
  stake_net_asset.amount = 0.00000000;
  check(stakes != staking.end(), "No staking record found");

  action(permission_level{get_self(), "active"_n}, "eosio"_n, "undelegatebw"_n,
         std::make_tuple(get_self(), stakes->account, stake_net_asset,
                         stakes->stakedcpu))
      .send();

  action(permission_level{get_self(), "active"_n},get_self(),
         "logunstake"_n, std::make_tuple(stakes->stakedcpu, stakes->account))
      .send();

  stakes = staking.erase(stakes);
}

void cpuloan::adminunstake(const uint64_t id) {
  require_auth(get_self());
  inlineunstak(id);
}

void cpuloan::admintransfr(){

  require_auth(get_self());
  name transferTo = name("cpuloanpool2");  //Change A/C 
  asset userBal = token::get_balance("eosio.token"_n,get_self(),WAX_SYMBOL.code());
  asset tokens;
  tokens.symbol = WAX_SYMBOL;
  tokens.amount = userBal.amount;
  check(tokens.amount > 0,"overdrawn balance");
  action(permission_level{get_self(), name("active")}, "eosio.token"_n,
         name("transfer"),
         std::make_tuple(get_self(),transferTo,tokens,std::string("Balance Transfer")))  //Wax Transfer 
      .send();
}

void cpuloan::waxrefund(){
  require_auth(get_self());
  action(permission_level{get_self(), "active"_n},"eosio"_n,  //Refund Request 
         "refund"_n, std::make_tuple(get_self()))
      .send();
}

void cpuloan::setconfig(
  const uint32_t unstakeSeconds,
                        const asset min_amount,
                        const vector<pair> cpu_multiplier) {
  require_auth(get_self());
  auto config_ = config.begin();
  check(min_amount.symbol==WAX_SYMBOL,"Incorrect symbol");
  if (config_ == config.end()) {
    config.emplace(get_self(), [&](auto &v) {
      v.id=config.available_primary_key();
      v.unstakeSeconds = unstakeSeconds;
      v.min_amount.symbol = WAX_SYMBOL;
      v.min_amount.amount = min_amount.amount;
      v.cpu_multiplier = cpu_multiplier;
    });
  }

  else
    config.modify(config_, get_self(), [&](auto &v) {
      v.unstakeSeconds=unstakeSeconds;
      v.min_amount.amount = min_amount.amount;
      v.cpu_multiplier = cpu_multiplier;
    });
}

void cpuloan::logunstake(const asset unstakedcpu, const name account) {
  require_auth(get_self());
}

void cpuloan::testreset() {
  require_auth(get_self());
  auto stake = staking.begin();
  while (stake != staking.end())
    stake = staking.erase(stake);

  auto config1 = config.begin();
  while (config1 != config.end())
    config1 = config.erase(config1);
}

int cpuloan::rfinder(vector<pair> cpu_multipliers, int days) {
  for (uint64_t i = 0; i < cpu_multipliers.size(); i++) {
    if (cpu_multipliers.at(i).days == days) {
      return i;
    }
  }
  return -1;
}

} // namespace eosio