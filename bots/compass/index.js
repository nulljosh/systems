import { chromium } from 'playwright';
import { execFileSync } from 'child_process';
import { parse } from 'csv-parse/sync';
import { homedir } from 'os';
import { join } from 'path';
import { existsSync } from 'fs';

const LOGIN_TIMEOUT = 30000;
const NAV_TIMEOUT = 15000;
const RECAPTCHA_WAIT = 60000;
const BASE_URL = 'https://www.compasscard.ca';
const PROFILE_DIR = join(homedir(), '.compass-profile');
const USER_AGENT = 'Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/131.0.0.0 Safari/537.36';
const RECAPTCHA_FRAME_PATTERNS = ['recaptcha/enterprise/anchor', 'recaptcha/api2/anchor'];

const CSV_COLUMN_MAP = {
  dateTime: ['DateTime', 'Date/Time', 'Date'],
  transaction: ['Transaction'],
  product: ['Product'],
  lineItem: ['LineItem', 'Line Item'],
  amount: ['Amount'],
  balance: ['Balance', 'BalanceDetails'],
  journeyId: ['JourneyId', 'Journey Id', 'Journey ID'],
};

function readKeychain(account) {
  try {
    return execFileSync(
      'security',
      ['find-generic-password', '-s', 'compass-card', '-a', account, '-w'],
      { encoding: 'utf8', timeout: 5000 }
    ).trim();
  } catch {
    throw new Error(
      `Failed to read "${account}" from Keychain. Run "node cli.js setup" first.`
    );
  }
}

function formatDate(date) {
  const mm = String(date.getMonth() + 1).padStart(2, '0');
  const dd = String(date.getDate()).padStart(2, '0');
  return `${mm}/${dd}/${date.getFullYear()}`;
}

function parseBalance(str) {
  if (!str) return null;
  const match = str.replace(/[$,]/g, '').trim().match(/[\d.]+/);
  return match ? parseFloat(match[0]) : null;
}

function mapCsvRow(row) {
  const result = {};
  for (const [key, candidates] of Object.entries(CSV_COLUMN_MAP)) {
    result[key] = candidates.reduce((val, col) => val || row[col] || '', '');
  }
  return result;
}

async function queryPage(page, selector, extract) {
  try {
    return await page.$eval(selector, extract) ?? null;
  } catch {
    return null;
  }
}

function hasProfile() {
  return existsSync(PROFILE_DIR);
}

export default class CompassCard {
  constructor(options = {}) {
    this.headed = options.headed || false;
    this.email = options.email || null;
    this.password = options.password || null;
    this.context = null;
    this.page = null;
  }

  async login() {
    if (!this.email) this.email = readKeychain('email');
    if (!this.password) this.password = readKeychain('password');

    if (!hasProfile() && !this.headed) {
      console.log('No saved browser profile. Launching in headed mode for reCAPTCHA.');
      this.headed = true;
    }

    this.context = await chromium.launchPersistentContext(PROFILE_DIR, {
      headless: !this.headed,
      viewport: { width: 1280, height: 800 },
      userAgent: USER_AGENT,
      args: [
        '--disable-blink-features=AutomationControlled',
        '--no-first-run',
        '--no-default-browser-check',
      ],
    });

    this.page = this.context.pages()[0] || await this.context.newPage();

    await this.page.goto(`${BASE_URL}/ManageCards`, {
      timeout: NAV_TIMEOUT,
      waitUntil: 'domcontentloaded',
    });

    if (this.page.url().includes('ManageCards') && await this.page.$('.card-number')) {
      await this.page.waitForLoadState('networkidle');
      return;
    }

    await this.page.goto(`${BASE_URL}/SignIn`, { timeout: NAV_TIMEOUT });
    await this.page.waitForLoadState('domcontentloaded');
    await this.page.waitForSelector('#Content_emailInfo_txtEmail', { timeout: 5000 });

    // ASP.NET WebForms requires keystroke events, not programmatic fill
    await this.page.locator('#Content_emailInfo_txtEmail').click();
    await this.page.locator('#Content_emailInfo_txtEmail').type(this.email, { delay: 20 });
    await this.page.locator('#Content_passwordInfo_txtPassword').click();
    await this.page.locator('#Content_passwordInfo_txtPassword').type(this.password, { delay: 20 });

    await this._handleRecaptcha();
    await this.page.locator('#Content_btnSignIn').click();

    try {
      await Promise.race([
        this.page.waitForURL('**/ManageCards**', { timeout: LOGIN_TIMEOUT }),
        this.page.waitForSelector('.card-number', { timeout: LOGIN_TIMEOUT }),
      ]);
    } catch {
      const errorText = await queryPage(
        this.page,
        '#Content_emailInfo_uv_txtEmail, #Content_passwordInfo_uv_txtPassword, .validation-summary-errors',
        el => el.textContent?.trim()
      );
      if (errorText) throw new Error(`Login failed: ${errorText}`);
      throw new Error('Login timed out. reCAPTCHA may have blocked. Try: node cli.js balance --headed');
    }

    await this.page.waitForLoadState('networkidle');
  }

  _findRecaptchaFrame() {
    return this.page.frames().find(
      f => RECAPTCHA_FRAME_PATTERNS.some(p => f.url().includes(p))
    );
  }

  async _handleRecaptcha() {
    const recaptchaFrame = this._findRecaptchaFrame();
    if (!recaptchaFrame) return;

    if (await recaptchaFrame.$('.recaptcha-checkbox-checked')) return;

    console.log('Clicking reCAPTCHA checkbox...');
    try {
      const checkboxBorder = await recaptchaFrame.$('.recaptcha-checkbox-border');
      if (checkboxBorder) {
        await checkboxBorder.click();
        await this.page.waitForTimeout(2000);
        if (await recaptchaFrame.$('.recaptcha-checkbox-checked')) {
          console.log('reCAPTCHA auto-solved.');
          return;
        }
      }
    } catch {
      // Auto-click failed, fall through to manual
    }

    if (this.headed) {
      console.log('reCAPTCHA image challenge appeared. Solve it in the browser window...');
      const startTime = Date.now();
      while (Date.now() - startTime < RECAPTCHA_WAIT) {
        // Re-find frame in case page navigated
        const frame = this._findRecaptchaFrame();
        if (frame && await frame.$('.recaptcha-checkbox-checked')) {
          console.log('reCAPTCHA solved.');
          await this.page.waitForTimeout(500);
          return;
        }
        await this.page.waitForTimeout(500);
      }
      throw new Error('reCAPTCHA was not solved within 60 seconds.');
    }

    throw new Error(
      'reCAPTCHA requires manual solving. Run: node cli.js balance --headed'
    );
  }

  async _ensureManageCards() {
    if (!this.page) throw new Error('Not logged in. Call login() first.');
    if (!this.page.url().includes('ManageCards')) {
      await this.page.goto(`${BASE_URL}/ManageCards`, { timeout: NAV_TIMEOUT });
      await this.page.waitForLoadState('networkidle');
    }
  }

  async getBalance() {
    await this._ensureManageCards();

    const cardNumber = await queryPage(this.page, '.card-number span', el => el.textContent?.trim());
    const balanceRaw = await queryPage(this.page, '.stored-values span', el => el.textContent?.trim());

    return {
      cardNumber,
      balance: parseBalance(balanceRaw),
      balanceRaw,
    };
  }

  async getTrips(days = 30) {
    await this._ensureManageCards();

    const cardSerial = await this.page.evaluate(() => {
      // Check named inputs first, then fall back to hidden inputs with serial-like names
      const selectors = [
        'input[name*="ccsn"]', 'input[id*="ccsn"]',
        '[data-ccsn]',
        'input[name$="hfSerialNo"]',
      ];
      for (const sel of selectors) {
        const el = document.querySelector(sel);
        if (el) return el.value || el.dataset?.ccsn || null;
      }
      for (const input of document.querySelectorAll('input[type="hidden"]')) {
        if (/Serial|ccsn|CCSN/.test(input.name)) return input.value;
      }
      return null;
    });

    if (!cardSerial) throw new Error('Could not find card serial number on page.');

    const end = new Date();
    const start = new Date();
    start.setDate(start.getDate() - days);

    const url =
      `${BASE_URL}/handlers/compasscardusagepdf.ashx` +
      `?type=2&start=${encodeURIComponent(formatDate(start))}&end=${encodeURIComponent(formatDate(end))}` +
      `&ccsn=${encodeURIComponent(cardSerial)}&csv=true`;

    const response = await this.page.request.get(url);
    if (!response.ok()) {
      throw new Error(`Failed to fetch trip history: HTTP ${response.status()}`);
    }

    const csvText = await response.text();
    if (!csvText.trim()) return [];

    let records;
    try {
      records = parse(csvText, {
        columns: true,
        skip_empty_lines: true,
        trim: true,
        relax_column_count: true,
      });
    } catch (err) {
      throw new Error(`Failed to parse trip CSV: ${err.message}`);
    }

    return records.map(mapCsvRow);
  }

  async close() {
    if (this.context) await this.context.close().catch(() => {});
    this.page = null;
    this.context = null;
  }
}

export { formatDate, parseBalance, mapCsvRow, hasProfile, PROFILE_DIR };
