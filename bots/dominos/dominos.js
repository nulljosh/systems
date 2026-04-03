const BASES = {
  ca: 'https://order.dominos.ca/power/',
  us: 'https://order.dominos.com/power/',
};

const SOURCE_URIS = {
  ca: 'order.dominos.ca',
  us: 'order.dominos.com',
};

const MARKETS = {
  ca: 'CANADA',
  us: 'UNITED_STATES',
};

const LANGUAGES = {
  ca: 'en',
  us: 'en',
};

class Order {
  constructor(api) {
    this._api = api;
    this.data = {
      Address: {},
      Coupons: [],
      CustomerID: '',
      Email: '',
      FirstName: '',
      LastName: '',
      LanguageCode: 'en',
      Market: MARKETS[api._region] || 'CANADA',
      OrderChannel: 'OLO',
      OrderMethod: 'Web',
      Payments: [],
      Phone: '',
      Products: [],
      ServiceMethod: 'Delivery',
      SourceOrganizationURI: SOURCE_URIS[api._region] || 'order.dominos.ca',
      StoreID: '',
      Tags: {},
      Version: '1.0',
      NoCombine: true,
      Partners: {},
      NewUser: true,
      metaData: {},
      OrderInfoCollection: [],
    };
  }

  _wrap() {
    return { Order: this.data };
  }

  setStore(storeId) {
    this.data.StoreID = storeId;
    return this;
  }

  setAddress(address) {
    this.data.Address = {
      Street: address.Street || address.street || '',
      City: address.City || address.city || '',
      Region: address.Region || address.region || '',
      PostalCode: address.PostalCode || address.postalCode || '',
      Type: address.Type || address.type || 'House',
    };
    return this;
  }

  setCustomer(customer) {
    this.data.FirstName = customer.firstName || '';
    this.data.LastName = customer.lastName || '';
    this.data.Email = customer.email || '';
    this.data.Phone = customer.phone || '';
    if (this._api._auth.customerId) {
      this.data.CustomerID = this._api._auth.customerId;
    }
    return this;
  }

  addItem({ Code, Qty = 1, Options = {}, isNew = true }) {
    this.data.Products.push({
      Code,
      Qty,
      Options,
      isNew,
      ID: this.data.Products.length + 1,
    });
    return this;
  }

  setPayment({ number, expiration, cvv, postalCode, amount, tipAmount = 0 }) {
    this.data.Payments = [
      {
        Type: 'CreditCard',
        Number: number,
        Expiration: expiration,
        SecurityCode: cvv,
        PostalCode: postalCode,
        Amount: amount,
        TipAmount: tipAmount,
        CardType: detectCardType(number),
      },
    ];
    return this;
  }

  async price() {
    const res = await this._api._post('price-order', this._wrap());
    if (res.Order) {
      Object.assign(this.data, res.Order);
    }
    return res;
  }

  async validate() {
    const res = await this._api._post('validate-order', this._wrap());
    return {
      valid: !res.Order?.Status || res.Order.Status === 1,
      StatusItems: res.Order?.StatusItems || [],
      ...res,
    };
  }

  async place({ confirmed = false } = {}) {
    if (!confirmed) {
      throw new Error('Order placement requires explicit confirmation: place({ confirmed: true }). Never call this without user approval.');
    }
    const res = await this._api._post('place-order', this._wrap());
    return res;
  }
}

function detectCardType(number) {
  if (!number) return 'VISA';
  const n = String(number).replace(/\D/g, '');
  if (/^4/.test(n)) return 'VISA';
  if (/^5[1-5]/.test(n)) return 'MASTERCARD';
  if (/^3[47]/.test(n)) return 'AMEX';
  if (/^6(?:011|5)/.test(n)) return 'DISCOVER';
  return 'VISA';
}

function decodeJwt(token) {
  try {
    const payload = token.split('.')[1];
    const json = Buffer.from(payload, 'base64url').toString('utf8');
    return JSON.parse(json);
  } catch {
    return null;
  }
}

export class DominosAPI {
  constructor({ region = 'ca', email, password } = {}) {
    this._region = region;
    this._email = email;
    this._password = password;
    this._base = BASES[region] || BASES.ca;
    this._market = MARKETS[region] || 'CANADA';
    this._lang = LANGUAGES[region] || 'en';
    this._auth = {
      token: null,
      customerId: null,
      _profile: null,
      profile: () => this._auth._profile,
    };

    this.auth = this._auth;
    this.stores = { find: this._findStores.bind(this) };
    this.menu = { get: this._getMenu.bind(this) };
    this.tracker = { byPhone: this._trackByPhone.bind(this) };
  }

  _headers(auth = false) {
    const h = {
      'DPZ-Language': this._lang,
      'DPZ-Market': this._market,
      'Content-Type': 'application/json',
      Accept: 'application/json',
    };
    if (auth && this._auth.token) {
      h['Authorization'] = `Bearer ${this._auth.token}`;
    }
    return h;
  }

  async _get(path, auth = false) {
    const url = `${this._base}${path}`;
    const res = await fetch(url, { headers: this._headers(auth) });
    if (!res.ok) {
      const text = await res.text().catch(() => '');
      throw new Error(`GET ${path} failed (${res.status}): ${text}`);
    }
    return res.json();
  }

  async _post(path, body, auth = false) {
    const url = `${this._base}${path}`;
    const res = await fetch(url, {
      method: 'POST',
      headers: this._headers(auth),
      body: JSON.stringify(body),
    });
    if (!res.ok) {
      const text = await res.text().catch(() => '');
      throw new Error(`POST ${path} failed (${res.status}): ${text}`);
    }
    return res.json();
  }

  async login() {
    const body = new URLSearchParams({
      grant_type: 'password',
      client_id: 'nolo',
      username: this._email,
      password: this._password,
    });

    const tokenUrl = 'https://api.dominos.ca/as/token.oauth2';
    const tokenRes = await fetch(tokenUrl, {
      method: 'POST',
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
      body: body.toString(),
    });

    if (!tokenRes.ok) {
      const text = await tokenRes.text().catch(() => '');
      throw new Error(`OAuth2 login failed (${tokenRes.status}): ${text}`);
    }

    const res = await tokenRes.json();

    if (res.access_token) {
      this._auth.token = res.access_token;
      this._auth._profile = decodeJwt(res.access_token);
    }

    if (this._auth._profile?.CustomerID) {
      this._auth.customerId = this._auth._profile.CustomerID;
    } else if (process.env.DOMINOS_CUSTOMER_ID) {
      this._auth.customerId = process.env.DOMINOS_CUSTOMER_ID;
    }

    return res;
  }

  createOrder() {
    return new Order(this);
  }

  async _findStores(address, serviceMethod = 'Delivery') {
    const addr = typeof address === 'string' ? address : formatAddress(address);
    const encoded = encodeURIComponent(addr);
    const path = `store-locator?type=${serviceMethod}&c=${encoded}&s=${encoded}`;
    const res = await this._get(path);
    const stores = res.Stores || [];
    return stores.map((s) => ({
      StoreID: s.StoreID,
      AddressDescription: s.AddressDescription,
      City: s.City,
      Distance: s.Distance,
      IsOnlineNow: s.IsOnlineNow,
      Phone: s.Phone,
      ...s,
    }));
  }

  async _getMenu(storeId) {
    const path = `store/${storeId}/menu?lang=${this._lang}&structured=true`;
    const res = await this._get(path);
    return {
      Products: res.Products || {},
      Coupons: res.Coupons || {},
      ...res,
    };
  }

  async _trackByPhone(phone) {
    const cleaned = String(phone).replace(/\D/g, '');
    const path = `tracker?Phone=${cleaned}`;
    return this._get(path);
  }

  async loyaltyStatus() {
    if (!this._auth.token) {
      throw new Error('Must login before checking loyalty status');
    }
    const res = await this._get('loyalty/points', true);
    return {
      points: res.Points ?? res.points ?? 0,
      pending: res.Pending ?? res.pending ?? 0,
      remaining: res.Remaining ?? res.remaining ?? 0,
      threshold: res.Threshold ?? res.threshold ?? 60,
      hasFreeReward: res.HasFreeReward ?? res.hasFreeReward ?? false,
      coupons: res.Coupons ?? res.coupons ?? [],
      ...res,
    };
  }
}

function formatAddress(addr) {
  const parts = [addr.Street, addr.City, addr.Region, addr.PostalCode].filter(Boolean);
  return parts.join(', ');
}
