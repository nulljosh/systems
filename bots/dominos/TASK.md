# Rebuild DominosAPI Library

Build `dominos.js` that exports a `DominosAPI` class matching this exact interface (reverse-engineered from the consumer in order.js):

## Constructor
```js
const api = new DominosAPI({ region: 'ca', email: 'user@email.com', password: 'pass' });
```

## Methods Required

### api.login()
- POST to login endpoint, store auth token + customerId
- Canada base: `https://order.dominos.ca/power/`
- US base: `https://order.dominos.com/power/`
- Login endpoint: `login` (POST with email/password)
- After login: `api.auth.customerId` must be available
- `api.auth.profile()` returns decoded JWT or profile object

### api.createOrder()
Returns an Order object with chainable methods:
- `.setStore(storeId)` - sets StoreID on order data
- `.setAddress(address)` - sets Address (object with Street, City, Region, PostalCode, Type)
- `.setCustomer(customer)` - sets customer info (firstName, lastName, email, phone)
- `.addItem({ Code, Qty, Options, isNew })` - adds product to order
- `.setPayment({ number, expiration, cvv, postalCode, amount, tipAmount })` - sets payment
- `.data` - direct access to raw order data object
- `.price()` - POST to price endpoint, returns { Order: { Amounts: { Customer, Tax, Payment }, Products, ServiceMethod, ... } }
- `.validate()` - POST to validate endpoint, returns { valid: bool, StatusItems }
- `.place()` - POST to place endpoint, returns { Order: { OrderID, Amounts, StoreID, ServiceMethod }, EstimatedWaitMinutes, Status }

### api.stores.find(address, serviceMethod)
- GET store locator endpoint
- Returns array of store objects with: StoreID, AddressDescription, City, Distance, IsOnlineNow, Phone

### api.menu.get(storeId)
- GET menu for store
- Returns { Products: { [code]: { Name, Description, Price, Tags } }, Coupons: { [code]: { Name, Description, Price, Amount, Tags } } }

### api.tracker.byPhone(phone)
- GET tracker endpoint
- Returns { AsOf, OrderStatuses: [{ StoreID, OrderID, OrderStatus, OrderDescription, StartTime, DriverName, ManagerName }] }

### api.loyaltyStatus()
- GET loyalty/points endpoint (requires auth)
- Returns { points, pending, remaining, threshold, hasFreeReward, coupons: [{ CouponCode, ... }] }

## API Details (Dominos Canada)
- Base URL Canada: `https://order.dominos.ca/power/`
- Headers: `DPZ-Language: en`, `DPZ-Market: CANADA`, `Content-Type: application/json`, `Accept: application/json`
- Store locator: GET `store-locator?type={serviceMethod}&c={address}&s={address}`
- Menu: GET `store/{storeId}/menu?lang=en&structured=true`
- Price: POST `price-order` with order body (must include `Order.Market: 'CANADA'`)
- Validate: POST `validate-order` with order body
- Place: POST `place-order` with order body
- Tracker: GET `tracker?Phone={phone}`
- Login: POST with credentials to get auth token
- Loyalty: needs auth token from login

## File
Write to: `~/Documents/Code/dominos/dominos.js`
- ESM module (export class DominosAPI)
- Use built-in fetch (Node 18+), no external deps
- Clean, production-quality code
- Handle Canadian market headers throughout

Test that `node -e "import { DominosAPI } from './dominos.js'; console.log('OK')"` works.
