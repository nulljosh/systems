import { execFileSync } from 'child_process';
import readline from 'readline';
import CompassCard, { hasProfile } from './index.js';

const args = process.argv.slice(2);
const command = args[0];
const headed = args.includes('--headed');

function getDaysArg() {
  const idx = args.indexOf('--days');
  if (idx !== -1 && args[idx + 1]) {
    const n = parseInt(args[idx + 1], 10);
    if (!isNaN(n) && n > 0 && n <= 365) return n;
    console.error('--days must be between 1 and 365');
    process.exit(1);
  }
  return 30;
}

function prompt(rl, question) {
  return new Promise(resolve => rl.question(question, resolve));
}

function printTable(trips) {
  if (!trips.length) {
    console.log('No trips found.');
    return;
  }

  const cols = ['dateTime', 'transaction', 'product', 'lineItem', 'amount', 'balance'];
  const headers = ['Date/Time', 'Transaction', 'Product', 'Line Item', 'Amount', 'Balance'];

  const widths = headers.map((h, i) => {
    const maxVal = trips.reduce((max, row) => Math.max(max, String(row[cols[i]] || '').length), 0);
    return Math.max(h.length, maxVal);
  });

  const row = (cells) => cells.map((c, i) => String(c || '').padEnd(widths[i])).join('  ');
  const sep = widths.map(w => '-'.repeat(w)).join('  ');

  console.log(row(headers));
  console.log(sep);
  for (const trip of trips) {
    console.log(row(cols.map(c => trip[c] || '')));
  }
}

async function run(fn) {
  const card = new CompassCard({ headed });
  try {
    await fn(card);
  } catch (err) {
    console.error('Error:', err.message);
    process.exitCode = 1;
  } finally {
    await card.close();
  }
}

async function cmdBalance() {
  await run(async (card) => {
    if (!hasProfile()) {
      console.log('First run -- browser will open. Solve the reCAPTCHA to establish trust.\n');
    }
    process.stdout.write('Logging in...\n');
    await card.login();
    process.stdout.write('Fetching balance...\n\n');
    const result = await card.getBalance();
    console.log(`Card   : ${result.cardNumber || 'N/A'}`);
    console.log(`Balance: ${result.balance !== null ? '$' + result.balance.toFixed(2) : result.balanceRaw || 'N/A'}`);
  });
}

async function cmdTrips() {
  const days = getDaysArg();
  await run(async (card) => {
    process.stdout.write('Logging in...\n');
    await card.login();
    process.stdout.write(`Fetching trips (last ${days} days)...\n\n`);
    const trips = await card.getTrips(days);
    printTable(trips);
    console.log(`\nTotal trips: ${trips.length}`);
  });
}

async function cmdSetup() {
  const rl = readline.createInterface({ input: process.stdin, output: process.stdout });

  let email, password;
  try {
    email = await prompt(rl, 'Compass Card email: ');
    password = await prompt(rl, 'Compass Card password: ');
  } finally {
    rl.close();
  }

  email = email.trim();
  password = password.trim();

  if (!email || !password) {
    console.error('Email and password are required.');
    process.exit(1);
  }

  try {
    execFileSync('security', [
      'add-generic-password', '-s', 'compass-card', '-a', 'email', '-w', email, '-U',
    ]);
    execFileSync('security', [
      'add-generic-password', '-s', 'compass-card', '-a', 'password', '-w', password, '-U',
    ]);
    console.log('Credentials saved to Keychain.');
    console.log('Run "node cli.js balance" to test (first run opens browser for reCAPTCHA).');
  } catch (err) {
    console.error('Failed to save credentials:', err.message);
    process.exit(1);
  }
}

function usage() {
  console.log([
    'Compass Card CLI',
    '',
    'Usage:',
    '  node cli.js balance              Check card balance',
    '  node cli.js trips [--days N]     Show trip history (default 30 days)',
    '  node cli.js setup                Store credentials in Keychain',
    '',
    'Options:',
    '  --headed                         Show browser window (required for reCAPTCHA)',
    '  --days N                         Number of days of history (default 30, max 365)',
    '',
    'First run opens the browser automatically so you can solve the reCAPTCHA.',
    'Subsequent runs reuse the saved browser profile and may work headless.',
  ].join('\n'));
}

switch (command) {
  case 'balance':
    cmdBalance();
    break;
  case 'trips':
    cmdTrips();
    break;
  case 'setup':
    cmdSetup();
    break;
  default:
    usage();
    if (command) process.exit(1);
}
