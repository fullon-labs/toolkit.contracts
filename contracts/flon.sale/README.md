# flon.sale (FullSale)

`flon.sale` is a fixed-price, inventory-backed FLON sale contract.  It is not
an AMM: a payment token transfer with `buy:<pair>` atomically forwards payment
to the configured revenue account and transfers the corresponding fixed-price
payout to the buyer.  If either transfer, the price calculation, inventory
limit, or token contract/symbol validation fails, the whole action
rolls back.

Each `pair` is independent and defines:

- one exact payment token contract and unit amount;
- one exact payout token contract and unit amount (normally FLON);
- a total sale allocation, total sold, and total payment received;
- a revenue account, time window, and pause switch.

## Lifecycle

1. Deploy `flon.sale` and give `flon.sale@active` permission for
   `flon.sale@eosio.code`, so it can make the two inline token transfers.
2. Call `init` from the contract account, choosing a multisig-controlled admin.
3. The admin calls `createoffer`. New offers start paused deliberately.
4. Fund the exact payout token with `fund:<pair>` up to `inventory_cap`.
5. Verify the `offers` row and contract token balance, then call `setpaused`
   with `false`.
6. Buyers transfer the configured payment token to `flon.sale` with
   `buy:<pair>`.

The price is represented as `payout_unit / payment_unit`. For example,
`1.000000 USDT -> 290.98070176 FLON` is a quote of 290.98070176 FLON per USDT.
Amounts round down in the buyer's favor only insofar as no fractional token
unit can be transferred; payments that round to zero payout are rejected.

Existing offers never change price. To publish another price, create a new
pair and pause the old one. This keeps the chain accounting auditable.

## Optional prepaid orders

Send the payment token with `order:<pair>` to place an order at the fixed price.
The payment stays in the contract until settlement. Query the `orders` table
in contract scope for the order ID and settled payment amount.

The admin calls `fillorder(order_id, payment)` to settle some or all of the
remaining payment using funded inventory. Each fill updates aggregate sales and
revenue. Orders can be filled after the offer's intake window ends. Pausing the
offer blocks fills. The buyer can call `cancelorder(order_id)` even while paused
to recover all unsettled payment. A full fill or cancellation erases the order.

`withdrawinv` is intentionally available only for a paused offer and can move
only funded-but-unsold payout inventory.
