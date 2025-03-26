#!/bin/bash

# testing.sh - Test script for dbserver
# Run with: bash testing.sh

echo "=== Starting Database Server Testing ==="

# Test 1: Basic functionality - start server, set/get/delete a key
echo "=== Test 1: Basic Set/Get/Delete functionality ==="
rm -f /tmp/data.*
PORT=5001
(sleep 10; echo quit) | ./dbserver $PORT &
sleep 1

echo "Setting key1 to value1..."
./dbtest --port=$PORT --set=key1 value1
echo "Getting key1..."
./dbtest --port=$PORT --get=key1
echo "Deleting key1..."
./dbtest --port=$PORT --delete=key1
echo "Getting key1 (should fail)..."
./dbtest --port=$PORT --get=key1
wait

# Test 2: Multiple keys
echo "=== Test 2: Multiple keys ==="
rm -f /tmp/data.*
PORT=5002
(sleep 10; echo quit) | ./dbserver $PORT &
sleep 1

echo "Setting multiple keys..."
./dbtest --port=$PORT --set=key1 "This is the first value"
./dbtest --port=$PORT --set=key2 "This is the second value"
./dbtest --port=$PORT --set=key3 "This is the third value"

echo "Getting multiple keys..."
./dbtest --port=$PORT --get=key1
./dbtest --port=$PORT --get=key2
./dbtest --port=$PORT --get=key3

echo "Deleting key2..."
./dbtest --port=$PORT --delete=key2

echo "Getting key2 (should fail)..."
./dbtest --port=$PORT --get=key2
wait

# Test 3: Load test - many concurrent requests
echo "=== Test 3: Load test with concurrent requests ==="
rm -f /tmp/data.*
PORT=5003
(sleep 15; echo quit) | ./dbserver $PORT &
sleep 1

echo "Sending 100 simultaneous requests with 5 threads..."
./dbtest --port=$PORT --count=100 --threads=5

wait

# Test 4: Simultaneous test
echo "=== Test 4: Simultaneous test ==="
rm -f /tmp/data.*
PORT=5004
(sleep 10; echo quit) | ./dbserver $PORT &
sleep 1

echo "Sending simultaneous mixed requests..."
./dbtest --port=$PORT --test

wait

# Test 5: Overwrite test
echo "=== Test 5: Overwrite test ==="
rm -f /tmp/data.*
PORT=5005
(sleep 10; echo quit) | ./dbserver $PORT &
sleep 1

echo "Setting key1 to value1..."
./dbtest --port=$PORT --set=key1 "Original value"
echo "Getting key1..."
./dbtest --port=$PORT --get=key1
echo "Overwriting key1..."
./dbtest --port=$PORT --set=key1 "New overwritten value"
echo "Getting key1 (should show new value)..."
./dbtest --port=$PORT --get=key1

wait

echo "=== All tests completed ==="