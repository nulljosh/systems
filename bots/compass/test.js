import { test } from 'node:test';
import assert from 'node:assert/strict';
import { parse } from 'csv-parse/sync';
import { formatDate, parseBalance, mapCsvRow, hasProfile, PROFILE_DIR } from './index.js';

test('formatDate produces MM/DD/YYYY', () => {
  assert.equal(formatDate(new Date(2024, 0, 5)), '01/05/2024');
});

test('formatDate pads single-digit month and day', () => {
  assert.equal(formatDate(new Date(2024, 8, 3)), '09/03/2024');
});

test('formatDate handles December 31', () => {
  assert.equal(formatDate(new Date(2023, 11, 31)), '12/31/2023');
});

test('formatDate handles January 1', () => {
  assert.equal(formatDate(new Date(2026, 0, 1)), '01/01/2026');
});

test('formatDate handles leap day', () => {
  assert.equal(formatDate(new Date(2024, 1, 29)), '02/29/2024');
});

test('parseBalance strips dollar sign', () => {
  assert.equal(parseBalance('$12.50'), 12.50);
});

test('parseBalance strips dollar sign and comma', () => {
  assert.equal(parseBalance('$1,234.56'), 1234.56);
});

test('parseBalance handles plain number string', () => {
  assert.equal(parseBalance('9.75'), 9.75);
});

test('parseBalance handles zero', () => {
  assert.equal(parseBalance('$0.00'), 0);
});

test('parseBalance handles large amounts', () => {
  assert.equal(parseBalance('$10,000.00'), 10000);
});

test('parseBalance returns null for empty string', () => {
  assert.equal(parseBalance(''), null);
});

test('parseBalance returns null for null input', () => {
  assert.equal(parseBalance(null), null);
});

test('parseBalance returns null for undefined', () => {
  assert.equal(parseBalance(undefined), null);
});

test('parseBalance returns null for non-numeric string', () => {
  assert.equal(parseBalance('N/A'), null);
});

test('parseBalance handles whitespace around value', () => {
  assert.equal(parseBalance('  $5.25  '), 5.25);
});

test('PROFILE_DIR points to home directory', () => {
  assert.ok(PROFILE_DIR.startsWith('/Users/'));
  assert.ok(PROFILE_DIR.endsWith('.compass-profile'));
});

test('hasProfile returns a boolean', () => {
  assert.equal(typeof hasProfile(), 'boolean');
});

test('mapCsvRow maps standard column names', () => {
  const row = {
    DateTime: '2024-01-15 08:30', Transaction: 'Tap in', Product: 'Stored Value',
    LineItem: 'Bus Zone 1', Amount: '-$2.40', Balance: '$15.60', JourneyId: 'ABC123',
  };
  const result = mapCsvRow(row);
  assert.equal(result.dateTime, '2024-01-15 08:30');
  assert.equal(result.transaction, 'Tap in');
  assert.equal(result.product, 'Stored Value');
  assert.equal(result.lineItem, 'Bus Zone 1');
  assert.equal(result.amount, '-$2.40');
  assert.equal(result.balance, '$15.60');
  assert.equal(result.journeyId, 'ABC123');
});

test('mapCsvRow maps alternate column names', () => {
  const row = {
    'Date/Time': '2024-02-01 17:45', Transaction: 'Tap in', Product: 'Stored Value',
    'Line Item': 'SkyTrain Zone 2', Amount: '-$3.65', BalanceDetails: '$20.00', 'Journey Id': 'XYZ789',
  };
  const result = mapCsvRow(row);
  assert.equal(result.dateTime, '2024-02-01 17:45');
  assert.equal(result.lineItem, 'SkyTrain Zone 2');
  assert.equal(result.balance, '$20.00');
  assert.equal(result.journeyId, 'XYZ789');
});

test('mapCsvRow returns empty strings for missing columns', () => {
  const result = mapCsvRow({});
  assert.equal(result.dateTime, '');
  assert.equal(result.transaction, '');
  assert.equal(result.journeyId, '');
});

test('CSV parsing with mapCsvRow produces correct trip objects', () => {
  const csv = [
    'DateTime,Transaction,Product,LineItem,Amount,Balance,JourneyId',
    '2024-01-15 08:30,Tap in,Stored Value,Bus Zone 1,-$2.40,$15.60,ABC123',
    '2024-01-15 09:00,Tap out,Stored Value,Bus Zone 1,$0.00,$15.60,ABC123',
  ].join('\n');

  const records = parse(csv, { columns: true, skip_empty_lines: true, trim: true });
  const trips = records.map(mapCsvRow);

  assert.equal(trips.length, 2);
  assert.equal(trips[0].dateTime, '2024-01-15 08:30');
  assert.equal(trips[0].transaction, 'Tap in');
  assert.equal(trips[1].transaction, 'Tap out');
});

test('CSV parsing with empty records returns empty array', () => {
  const csv = 'DateTime,Transaction,Product,LineItem,Amount,Balance,JourneyId\n';
  const records = parse(csv, { columns: true, skip_empty_lines: true, trim: true });
  assert.equal(records.length, 0);
});

test('CSV parsing handles extra whitespace in values', () => {
  const csv = [
    'DateTime,Transaction,Product,LineItem,Amount,Balance,JourneyId',
    '  2024-03-01 08:00 , Tap in , Stored Value , Bus Zone 1 , -$2.40 , $15.60 , DEF456 ',
  ].join('\n');

  const records = parse(csv, { columns: true, skip_empty_lines: true, trim: true });
  assert.equal(records[0]['DateTime'], '2024-03-01 08:00');
  assert.equal(records[0]['Transaction'], 'Tap in');
});

test('CSV parsing handles transfer transactions', () => {
  const csv = [
    'DateTime,Transaction,Product,LineItem,Amount,Balance,JourneyId',
    '2024-01-15 08:30,Tap in at transfer,Monthly Pass,Bus Zone 1,$0.00,$0.00,TRF001',
  ].join('\n');

  const records = parse(csv, { columns: true, skip_empty_lines: true, trim: true });
  const trip = mapCsvRow(records[0]);
  assert.equal(trip.transaction, 'Tap in at transfer');
  assert.equal(trip.amount, '$0.00');
});
