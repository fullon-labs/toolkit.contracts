#include <flon.sale.hpp>

#include <eosio/action.hpp>
#include <eosio/system.hpp>

#include <vector>

using namespace eosio;
using std::string;
using std::vector;

namespace {
constexpr name ACTIVE_PERMISSION = "active"_n;

vector<string> split(const string& value, char delimiter) {
   vector<string> result;
   size_t start = 0;
   while (true) {
      const size_t end = value.find(delimiter, start);
      result.emplace_back(value.substr(start, end == string::npos ? string::npos : end - start));
      if (end == string::npos) {
         break;
      }
      start = end + 1;
   }
   return result;
}

} // namespace

flonsale::flonsale(name receiver, name code, datastream<const char*> ds)
   : contract(receiver, code, ds),
     _config(receiver, receiver.value),
     _offers(receiver, receiver.value) {}

flonsale::config_row flonsale::config() {
   check(_config.exists(), "flon.sale is not initialized");
   return _config.get();
}

void flonsale::require_initialized() {
   (void)config();
}

void flonsale::require_admin() {
   require_auth(config().admin);
}

void flonsale::init(name admin) {
   require_auth(get_self());
   check(is_account(admin), "admin account does not exist");
   check(!_config.exists(), "flon.sale is already initialized");
   _config.set(config_row{admin, name{}}, get_self());
}

void flonsale::createoffer(
   name pair,
   extended_asset payment_unit,
   extended_asset payout_unit,
   asset inventory_cap,
   name revenue_account,
   time_point_sec starts_at,
   time_point_sec ends_at,
   string note) {
   require_initialized();
   require_admin();
   check(pair.value != 0, "pair is required");
   check(payment_unit.contract != get_self(), "payment token contract cannot be flon.sale");
   check(payment_unit.quantity.amount > 0, "payment_unit must be positive");
   check(payout_unit.quantity.amount > 0, "payout_unit must be positive");
   check(inventory_cap.symbol == payout_unit.quantity.symbol && inventory_cap.amount > 0,
         "inventory_cap must be positive and use the payout symbol");
   check(is_account(payment_unit.contract), "payment token contract does not exist");
   check(is_account(payout_unit.contract), "payout token contract does not exist");
   check(is_account(revenue_account), "revenue account does not exist");
   check(ends_at.sec_since_epoch() == 0 || starts_at < ends_at, "end time must be later than start time");
   check(note.size() <= 256, "note is too long");
   check(_offers.find(pair.value) == _offers.end(), "offer already exists; create a new pair for a new price");

   _offers.emplace(get_self(), [&](auto& row) {
      row.pair = pair;
      row.payment_unit = payment_unit;
      row.payout_unit = payout_unit;
      row.inventory_cap = inventory_cap;
      row.reserved = asset{0, payout_unit.quantity.symbol};
      row.funded = asset{0, payout_unit.quantity.symbol};
      row.sold = asset{0, payout_unit.quantity.symbol};
      row.received = asset{0, payment_unit.quantity.symbol};
      row.revenue_account = revenue_account;
      row.starts_at = starts_at;
      row.ends_at = ends_at;
      row.paused = true;
      row.note = note;
   });
}

void flonsale::setpaused(name pair, bool paused) {
   require_initialized();
   require_admin();
   const auto itr = _offers.require_find(pair.value, "offer does not exist");
   _offers.modify(itr, get_self(), [&](auto& row) { row.paused = paused; });
}

void flonsale::transfer_token(name token_contract, name to, asset quantity, const string& memo) const {
   action(
      permission_level{get_self(), ACTIVE_PERMISSION},
      token_contract,
      "transfer"_n,
      std::make_tuple(get_self(), to, quantity, memo)
   ).send();
}

void flonsale::withdrawinv(name pair, name to, asset quantity, string memo) {
   require_initialized();
   require_admin();
   check(is_account(to), "recipient account does not exist");
   check(memo.size() <= 256, "memo is too long");
   const auto itr = _offers.require_find(pair.value, "offer does not exist");
   check(itr->paused, "pause the offer before withdrawing inventory");
   check(quantity.symbol == itr->payout_unit.quantity.symbol && quantity.amount > 0,
         "quantity must be positive and use the payout symbol");
   const asset unsold = itr->funded - itr->sold;
   check(quantity <= unsold, "cannot withdraw sold or unfunded inventory");

   _offers.modify(itr, get_self(), [&](auto& row) { row.funded -= quantity; });
   transfer_token(itr->payout_unit.contract, to, quantity, memo);
}

void flonsale::proposeadmin(name new_admin) {
   require_initialized();
   require_admin();
   check(is_account(new_admin), "new admin account does not exist");
   check(new_admin != config().admin, "new admin is already the admin");
   auto state = config();
   state.proposed_admin = new_admin;
   _config.set(state, get_self());
}

void flonsale::acceptadmin() {
   auto state = config();
   check(state.proposed_admin.value != 0, "no admin transfer has been proposed");
   require_auth(state.proposed_admin);
   state.admin = state.proposed_admin;
   state.proposed_admin = name{};
   _config.set(state, get_self());
}

void flonsale::cancelorder(uint64_t order_id) {
   orders_table orders(get_self(), get_self().value);
   const auto pending = orders.get(order_id, "order does not exist");
   require_auth(pending.buyer);
   const auto& offer = _offers.get(pending.pair.value, "offer does not exist");
   orders.erase(orders.find(order_id));
   transfer_token(offer.payment_unit.contract, pending.buyer,
                  pending.payment - pending.settled_payment, "flon.sale refund");
}

void flonsale::fillorder(uint64_t order_id, asset payment) {
   require_admin();
   orders_table orders(get_self(), get_self().value);
   const auto order_itr = orders.require_find(order_id, "order does not exist");
   const auto pending = *order_itr;
   const auto offer_itr = _offers.require_find(pending.pair.value, "offer does not exist");
   check(!offer_itr->paused, "offer is paused");
   check(payment.symbol == pending.payment.symbol && payment.amount > 0,
         "invalid fill payment");
   check(payment <= pending.payment - pending.settled_payment, "fill exceeds remaining payment");
   const auto cumulative = pending.settled_payment + payment;
   const auto payout_for = [&](int64_t amount) {
      return static_cast<int64_t>(static_cast<__int128>(amount) * offer_itr->payout_unit.quantity.amount
                                  / offer_itr->payment_unit.quantity.amount);
   };
   const asset payout{payout_for(cumulative.amount) - payout_for(pending.settled_payment.amount),
                     pending.payout.symbol};
   check(payout.amount > 0, "fill payout rounds to zero");
   if (cumulative == pending.payment) {
      orders.erase(order_itr);
   } else {
      check(payout_for(cumulative.amount) < pending.payout.amount,
            "fill would leave a remainder with no payout");
      orders.modify(order_itr, same_payer, [&](auto& row) { row.settled_payment = cumulative; });
   }
   settle(offer_itr, pending.buyer, payment, payout);
}

void flonsale::ontransfer(name from, name to, asset quantity, string memo) {
   if (from == get_self() || to != get_self()) {
      return;
   }
   require_initialized();
   check(quantity.amount > 0, "transfer quantity must be positive");
   check(memo.size() <= 256, "memo is too long");

   const auto parts = split(memo, ':');
   check(parts.size() == 2, "memo must be 'fund:<pair>', 'buy:<pair>', or 'order:<pair>'");
   const name pair = name{parts[1]};
   const auto itr = _offers.require_find(pair.value, "offer does not exist");
   const name token_contract = get_first_receiver();

   if (parts[0] == "fund") {
      check(token_contract == itr->payout_unit.contract && quantity.symbol == itr->payout_unit.quantity.symbol,
            "funding must use the configured payout token");
      const asset remaining_cap = itr->inventory_cap - itr->funded;
      check(quantity <= remaining_cap, "funding exceeds inventory cap");
      _offers.modify(itr, get_self(), [&](auto& row) { row.funded += quantity; });
      return;
   }

   const bool place_order = parts[0] == "order";
   check(parts[0] == "buy" || place_order, "unknown sale action in memo");
   check(token_contract == itr->payment_unit.contract && quantity.symbol == itr->payment_unit.quantity.symbol,
         "payment must use the configured payment token");
   check(!itr->paused, "offer is paused");
   const auto now = time_point_sec(current_time_point());
   check(now >= itr->starts_at, "offer has not started");
   check(itr->ends_at.sec_since_epoch() == 0 || now < itr->ends_at, "offer has ended");

   const __int128 numerator = static_cast<__int128>(quantity.amount) * itr->payout_unit.quantity.amount;
   const auto payout_amount = numerator / itr->payment_unit.quantity.amount;
   check(payout_amount > 0, "payment amount is below the smallest purchasable payout");
   check(payout_amount <= asset::max_amount, "payout amount exceeds asset limit");
   const asset payout{static_cast<int64_t>(payout_amount), itr->payout_unit.quantity.symbol};
   if (place_order) {
      check(payout <= itr->inventory_cap - itr->sold, "inventory cap reached");
      require_auth(from);
      order_sequence_table sequence(get_self(), get_self().value);
      auto counter = sequence.get_or_default();
      check(counter.last_id < UINT64_MAX, "order IDs exhausted");
      ++counter.last_id;
      sequence.set(counter, get_self());
      orders_table orders(get_self(), get_self().value);
      orders.emplace(get_self(), [&](auto& row) {
         row = order{counter.last_id, from, pair, quantity, payout, asset{0, quantity.symbol}};
      });
      return;
   }
   settle(itr, from, quantity, payout);
}

void flonsale::settle(offers_table::const_iterator itr, name from, asset quantity, asset payout) {
   check(payout <= itr->funded - itr->sold, "insufficient funded inventory");
   check(payout <= itr->inventory_cap - itr->sold, "inventory cap reached");

   _offers.modify(itr, get_self(), [&](auto& row) {
      row.sold += payout;
      row.received += quantity;
   });

   if (itr->revenue_account != get_self()) {
      transfer_token(itr->payment_unit.contract, itr->revenue_account, quantity, "flon.sale payment " + itr->pair.to_string());
   }
   transfer_token(itr->payout_unit.contract, from, payout, "flon.sale purchase " + itr->pair.to_string());
}
