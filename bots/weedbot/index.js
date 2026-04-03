#!/usr/bin/env node
import { readFileSync, writeFileSync, mkdirSync, existsSync, appendFileSync } from 'fs';
import { join, dirname } from 'path';
import { fileURLToPath } from 'url';

const __dirname = dirname(fileURLToPath(import.meta.url));
const STASH_FILE = join(__dirname, 'data', 'stash.json');
const DEALS_FILE = join(__dirname, 'data', 'deals.json');
const PURCHASES_FILE = join(__dirname, 'data', 'purchases.json');
const LOG_FILE = join(__dirname, 'logs', 'usage.log');

// --- helpers ---

function loadJSON(file) {
  if (!existsSync(file)) return [];
  return JSON.parse(readFileSync(file, 'utf8'));
}

function saveJSON(file, data) {
  mkdirSync(dirname(file), { recursive: true });
  writeFileSync(file, JSON.stringify(data, null, 2));
}

function log(action, strain, amount) {
  mkdirSync(dirname(LOG_FILE), { recursive: true });
  appendFileSync(LOG_FILE, `${new Date().toISOString()} | ${action} | ${strain} | ${amount}g\n`);
}

function ppg(price, grams) {
  return (price / grams).toFixed(2);
}

// --- stash ---

function showStash() {
  const stash = loadJSON(STASH_FILE);
  if (!stash.length) { console.log('Stash is empty.'); return; }
  for (const s of stash) {
    const oz = (s.amount / 28).toFixed(1);
    console.log(`  ${s.strain}: ${s.amount}g (${oz}oz)`);
  }
  const total = stash.reduce((t, s) => t + s.amount, 0);
  if (stash.length > 1) console.log(`  Total: ${total}g (${(total / 28).toFixed(1)}oz)`);
  if (total <= 3.5) console.log('  !! CRITICAL -- under an eighth. Re-up now.');
  else if (total <= 7) console.log('  ! Low -- under a quarter.');
}

function setStash(strain, amount) {
  const stash = loadJSON(STASH_FILE);
  const idx = stash.findIndex(s => s.strain.toLowerCase() === strain.toLowerCase());
  if (idx >= 0) stash[idx].amount = amount;
  else stash.push({ strain, amount });
  saveJSON(STASH_FILE, stash);
  log('set', strain, amount);
  console.log(`Set ${strain} to ${amount}g.`);
}

function addStash(strain, amount) {
  const stash = loadJSON(STASH_FILE);
  const idx = stash.findIndex(s => s.strain.toLowerCase() === strain.toLowerCase());
  if (idx >= 0) stash[idx].amount += amount;
  else stash.push({ strain, amount });
  saveJSON(STASH_FILE, stash);
  log('add', strain, amount);
  console.log(`Added ${amount}g ${strain}. Now ${idx >= 0 ? stash[idx].amount : amount}g.`);
}

function smoke(amount, strain) {
  const stash = loadJSON(STASH_FILE);
  if (!stash.length) { console.log('Stash is empty.'); return; }
  let idx = 0;
  if (strain) {
    idx = stash.findIndex(s => s.strain.toLowerCase() === strain.toLowerCase());
    if (idx < 0) { console.log(`No ${strain} in stash.`); return; }
  }
  stash[idx].amount = Math.max(0, stash[idx].amount - amount);
  const remaining = stash[idx].amount;
  log('smoke', stash[idx].strain, amount);
  if (remaining <= 0) {
    const name = stash[idx].strain;
    stash.splice(idx, 1);
    saveJSON(STASH_FILE, stash);
    console.log(`Smoked ${amount}g. ${name} is gone.`);
  } else {
    saveJSON(STASH_FILE, stash);
    console.log(`Smoked ${amount}g. ${remaining}g ${stash[idx].strain} left.`);
    if (remaining <= 3.5) console.log('!! CRITICAL -- under an eighth. Re-up now.');
    else if (remaining <= 7) console.log('! Low -- under a quarter.');
  }
}

// --- deals ---

function showDeals() {
  const deals = loadJSON(DEALS_FILE);
  if (!deals.length) { console.log('No deals saved. Use: deals add <source> <strain> <price> <grams> [url]'); return; }
  const sorted = [...deals].sort((a, b) => a.ppg - b.ppg);
  console.log('Oz Deals (best price-per-gram first):');
  for (const d of sorted) {
    console.log(`  $${d.price}/${d.grams}g ($${d.ppg}/g) -- ${d.strain} @ ${d.source}${d.url ? ' ' + d.url : ''}`);
  }
}

function addDeal(source, strain, price, grams, url) {
  const deals = loadJSON(DEALS_FILE);
  const deal = {
    source,
    strain,
    price: parseFloat(price),
    grams: parseFloat(grams),
    ppg: ppg(price, grams),
    url: url || null,
    added: new Date().toISOString().split('T')[0]
  };
  deals.push(deal);
  saveJSON(DEALS_FILE, deals);
  console.log(`Added: ${strain} @ ${source} -- $${price}/${grams}g ($${deal.ppg}/g)`);
}

function rmDeal(index) {
  const deals = loadJSON(DEALS_FILE);
  const i = parseInt(index) - 1;
  if (i < 0 || i >= deals.length) { console.log(`Invalid index. 1-${deals.length}.`); return; }
  const removed = deals.splice(i, 1)[0];
  saveJSON(DEALS_FILE, deals);
  console.log(`Removed: ${removed.strain} @ ${removed.source}`);
}

function bestDeals(n) {
  const deals = loadJSON(DEALS_FILE);
  if (!deals.length) { console.log('No deals saved.'); return; }
  const sorted = [...deals].sort((a, b) => a.ppg - b.ppg);
  const top = sorted.slice(0, n || 5);
  console.log(`Top ${top.length} deals:`);
  for (let i = 0; i < top.length; i++) {
    const d = top[i];
    console.log(`  ${i + 1}. $${d.ppg}/g -- ${d.strain} @ ${d.source} ($${d.price}/${d.grams}g)`);
  }
}

// --- purchases ---

function buy(strain, grams, price, source) {
  const g = parseFloat(grams);
  const p = parseFloat(price);
  const purchases = loadJSON(PURCHASES_FILE);
  purchases.push({
    strain,
    grams: g,
    price: p,
    ppg: ppg(p, g),
    source: source || null,
    date: new Date().toISOString().split('T')[0]
  });
  saveJSON(PURCHASES_FILE, purchases);
  addStash(strain, g);
  log('buy', strain, g);
  console.log(`Logged purchase: ${g}g ${strain} for $${p} ($${ppg(p, g)}/g)${source ? ' from ' + source : ''}`);
}

function showHistory() {
  const purchases = loadJSON(PURCHASES_FILE);
  if (!purchases.length) { console.log('No purchase history.'); return; }
  console.log('Purchase history:');
  let totalSpent = 0, totalGrams = 0;
  for (const p of purchases) {
    console.log(`  ${p.date} -- ${p.grams}g ${p.strain} $${p.price} ($${p.ppg}/g)${p.source ? ' @ ' + p.source : ''}`);
    totalSpent += p.price;
    totalGrams += p.grams;
  }
  console.log(`  ---`);
  console.log(`  Total: ${totalGrams}g for $${totalSpent.toFixed(2)} (avg $${ppg(totalSpent, totalGrams)}/g)`);
}

// --- CLI ---

const [,, cmd, ...args] = process.argv;

switch (cmd) {
  case 'stash':
    if (!args.length) { showStash(); break; }
    switch (args[0]) {
      case 'set': setStash(args[1], parseFloat(args[2])); break;
      case 'add': addStash(args[1], parseFloat(args[2])); break;
      case 'smoke': smoke(parseFloat(args[1]), args[2]); break;
      default: console.log('Usage: stash [set|add|smoke] <strain> <amount>');
    }
    break;
  case 'deals':
    if (!args.length) { showDeals(); break; }
    switch (args[0]) {
      case 'add': addDeal(args[1], args[2], args[3], args[4], args[5]); break;
      case 'rm': rmDeal(args[1]); break;
      case 'best': bestDeals(parseInt(args[1]) || 5); break;
      default: console.log('Usage: deals [add|rm|best]');
    }
    break;
  case 'buy':
    buy(args[0], args[1], args[2], args[3]);
    break;
  case 'history':
    showHistory();
    break;
  default:
    console.log(`weedbot -- stash tracker + deals + purchase history

  stash                          show current stash
  stash set <strain> <grams>     set strain amount
  stash add <strain> <grams>     add to strain
  stash smoke <grams> [strain]   smoke from stash

  deals                          list all deals (sorted by $/g)
  deals add <src> <strain> <$> <g> [url]   add a deal
  deals rm <#>                   remove deal by number
  deals best [n]                 top N deals (default 5)

  buy <strain> <g> <$> [source]  log purchase + add to stash
  history                        purchase history + totals`);
}
