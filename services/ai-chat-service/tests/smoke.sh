#!/usr/bin/env bash
set -euo pipefail

project="cyanisok-smoke"
compose=(
  docker compose
  --project-name "$project"
  -f docker-compose.yml
  -f tests/docker-compose.smoke.yml
)

export MYSQL_ROOT_PASSWORD="${MYSQL_ROOT_PASSWORD:-smoke-root-password}"
export MYSQL_PASSWORD="${MYSQL_PASSWORD:-smoke-app-password}"
export DASHSCOPE_API_KEY="${DASHSCOPE_API_KEY:-smoke-api-key}"

cleanup() {
  "${compose[@]}" down -v --remove-orphans
}
trap cleanup EXIT

"${compose[@]}" up -d --build mysql mock-ai app

for attempt in {1..60}; do
  if curl -fsS http://127.0.0.1:8080/ready >/dev/null; then
    break
  fi
  if [[ "$attempt" == "60" ]]; then
    "${compose[@]}" logs app
    exit 1
  fi
  sleep 2
done

run_id="$(date +%s)"
username="smoke_$run_id"
tmp_dir="$(mktemp -d)"
cookie_file="$tmp_dir/user-1.cookie"
trap 'rm -rf "$tmp_dir"; cleanup' EXIT

curl -fsS \
  -H "Content-Type: application/json" \
  -d "{\"username\":\"$username\",\"password\":\"smoke-password\"}" \
  http://127.0.0.1:8080/register >/dev/null

curl -fsS \
  -c "$cookie_file" \
  -H "Content-Type: application/json" \
  -d "{\"username\":\"$username\",\"password\":\"smoke-password\"}" \
  http://127.0.0.1:8080/login >/dev/null

stream="$(
  curl -fsS -N \
    -b "$cookie_file" \
    -H "Content-Type: application/json" \
    -d '{"question":"hello"}' \
    http://127.0.0.1:8080/chat/send
)"
grep -q "event: done" <<<"$stream"

history="$(
  curl -fsS \
    -b "$cookie_file" \
    -H "Content-Type: application/json" \
    -d '{}' \
    http://127.0.0.1:8080/chat/history
)"
grep -q "mock response: hello" <<<"$history"

upload_status="$(
  curl -sS \
    -o "$tmp_dir/upload-disabled.body" \
    -w "%{http_code}" \
    -b "$cookie_file" \
    -H "Content-Type: application/json" \
    -d '{"filename":"test.png","image":"aW52YWxpZA=="}' \
    http://127.0.0.1:8080/upload/send
)"
[[ "$upload_status" == "503" ]]

race_username="race_$run_id"
for request in 1 2; do
  (
    curl -sS \
      -o "$tmp_dir/race-$request.body" \
      -w "%{http_code}" \
      -H "Content-Type: application/json" \
      -d "{\"username\":\"$race_username\",\"password\":\"smoke-password\"}" \
      http://127.0.0.1:8080/register >"$tmp_dir/race-$request.code"
  ) &
done
wait
race_codes="$(sort "$tmp_dir"/race-*.code | tr '\n' ' ')"
[[ "$race_codes" == "200 409 " ]]

for user_number in 2 3; do
  extra_username="smoke_${run_id}_$user_number"
  curl -fsS \
    -H "Content-Type: application/json" \
    -d "{\"username\":\"$extra_username\",\"password\":\"smoke-password\"}" \
    http://127.0.0.1:8080/register >/dev/null
  curl -fsS \
    -c "$tmp_dir/user-$user_number.cookie" \
    -H "Content-Type: application/json" \
    -d "{\"username\":\"$extra_username\",\"password\":\"smoke-password\"}" \
    http://127.0.0.1:8080/login >/dev/null
done

curl -fsS -N \
  -b "$cookie_file" \
  -H "Content-Type: application/json" \
  -d '{"question":"hold-one"}' \
  http://127.0.0.1:8080/chat/send >"$tmp_dir/hold-one.stream" &
hold_one_pid=$!
sleep 0.3

same_user_code="$(
  curl -sS \
    -o "$tmp_dir/same-user.body" \
    -w "%{http_code}" \
    -b "$cookie_file" \
    -H "Content-Type: application/json" \
    -d '{"question":"duplicate"}' \
    http://127.0.0.1:8080/chat/send
)"
[[ "$same_user_code" == "409" ]]

curl -fsS -N \
  -b "$tmp_dir/user-2.cookie" \
  -H "Content-Type: application/json" \
  -d '{"question":"hold-two"}' \
  http://127.0.0.1:8080/chat/send >"$tmp_dir/hold-two.stream" &
hold_two_pid=$!
sleep 0.3

queue_full_code="$(
  curl -sS \
    -o "$tmp_dir/queue-full.body" \
    -w "%{http_code}" \
    -b "$tmp_dir/user-3.cookie" \
    -H "Content-Type: application/json" \
    -d '{"question":"queue-full"}' \
    http://127.0.0.1:8080/chat/send
)"
[[ "$queue_full_code" == "503" ]]

wait "$hold_one_pid"
wait "$hold_two_pid"
grep -q "event: done" "$tmp_dir/hold-one.stream"
grep -q "event: done" "$tmp_dir/hold-two.stream"

failure_stream="$(
  curl -fsS -N \
    -b "$cookie_file" \
    -H "Content-Type: application/json" \
    -d '{"question":"fail"}' \
    http://127.0.0.1:8080/chat/send
)"
grep -q "event: error" <<<"$failure_stream"

history_after_failure="$(
  curl -fsS \
    -b "$cookie_file" \
    -H "Content-Type: application/json" \
    -d '{}' \
    http://127.0.0.1:8080/chat/history
)"
python3 -c '
import json
import sys

history = json.load(sys.stdin)["history"]
assert any(item["is_user"] and item["content"] == "fail" for item in history)
assert not any((not item["is_user"]) and "mock response: fail" in item["content"] for item in history)
' <<<"$history_after_failure"

truncated_stream="$(
  curl -fsS -N \
    -b "$cookie_file" \
    -H "Content-Type: application/json" \
    -d '{"question":"truncated"}' \
    http://127.0.0.1:8080/chat/send
)"
grep -q "event: delta" <<<"$truncated_stream"
grep -q "event: error" <<<"$truncated_stream"
if grep -q "event: done" <<<"$truncated_stream"; then
  exit 1
fi

history_after_truncation="$(
  curl -fsS \
    -b "$cookie_file" \
    -H "Content-Type: application/json" \
    -d '{}' \
    http://127.0.0.1:8080/chat/history
)"
python3 -c '
import json
import sys

history = json.load(sys.stdin)["history"]
assert any(item["is_user"] and item["content"] == "truncated" for item in history)
assert not any((not item["is_user"]) and "mock response: truncated" in item["content"] for item in history)
' <<<"$history_after_truncation"

set +e
curl -sS -N \
  --max-time 0.3 \
  -b "$cookie_file" \
  -H "Content-Type: application/json" \
  -d '{"question":"disconnect"}' \
  http://127.0.0.1:8080/chat/send >/dev/null
disconnect_status=$?
set -e
[[ "$disconnect_status" == "28" ]]
sleep 2

history_after_disconnect="$(
  curl -fsS \
    -b "$cookie_file" \
    -H "Content-Type: application/json" \
    -d '{}' \
    http://127.0.0.1:8080/chat/history
)"
python3 -c '
import json
import sys

history = json.load(sys.stdin)["history"]
assert any(item["is_user"] and item["content"] == "disconnect" for item in history)
assert not any((not item["is_user"]) and "mock response: disconnect" in item["content"] for item in history)
' <<<"$history_after_disconnect"

db_user="${MYSQL_USER:-chatuser}"
db_name="${MYSQL_DATABASE:-chatserver}"
user_id="$(
  "${compose[@]}" exec -T \
    -e MYSQL_PWD="$MYSQL_PASSWORD" \
    mysql mysql -u"$db_user" -N -s "$db_name" \
    -e "SELECT id FROM users WHERE username = '$username' LIMIT 1"
)"

base_timestamp=$(( $(date +%s) * 1000 - 1000000 ))
message_values=""
for index in $(seq 0 204); do
  if [[ -n "$message_values" ]]; then
    message_values+=","
  fi
  message_values+="($user_id,'$username',1,'seed-$index',$((base_timestamp + index)))"
done
"${compose[@]}" exec -T \
  -e MYSQL_PWD="$MYSQL_PASSWORD" \
  mysql mysql -u"$db_user" "$db_name" \
  -e "INSERT INTO chat_message (user_id, username, is_user, content, ts) VALUES $message_values"

context_stream="$(
  curl -fsS -N \
    -b "$cookie_file" \
    -H "Content-Type: application/json" \
    -d '{"question":"context-check"}' \
    http://127.0.0.1:8080/chat/send
)"
grep -q "context=40" <<<"$context_stream"

limited_history="$(
  curl -fsS \
    -b "$cookie_file" \
    -H "Content-Type: application/json" \
    -d '{}' \
    http://127.0.0.1:8080/chat/history
)"
python3 -c '
import json
import sys

history = json.load(sys.stdin)["history"]
contents = [item["content"] for item in history]
assert len(history) == 200
assert "seed-0" not in contents
assert "seed-204" in contents
assert any("context=40" in content for content in contents)
' <<<"$limited_history"

"${compose[@]}" exec -T \
  -e MYSQL_PWD="$MYSQL_PASSWORD" \
  mysql mysql -u"$db_user" "$db_name" \
  -e "INSERT INTO chat_message (user_id, username, is_user, content, ts) VALUES ($user_id,'$username',1,'expired-marker',0)"
"${compose[@]}" restart app >/dev/null

for attempt in {1..30}; do
  if curl -fsS http://127.0.0.1:8080/ready >/dev/null; then
    break
  fi
  if [[ "$attempt" == "30" ]]; then
    "${compose[@]}" logs app
    exit 1
  fi
  sleep 1
done

expired_count="$(
  "${compose[@]}" exec -T \
    -e MYSQL_PWD="$MYSQL_PASSWORD" \
    mysql mysql -u"$db_user" -N -s "$db_name" \
    -e "SELECT COUNT(*) FROM chat_message WHERE content = 'expired-marker'"
)"
[[ "$expired_count" == "0" ]]

export SMOKE_IMAGE_RECOGNITION_ENABLED=true
"${compose[@]}" up -d --force-recreate app >/dev/null
for attempt in {1..30}; do
  if curl -fsS http://127.0.0.1:8080/ready >/dev/null; then
    break
  fi
  if [[ "$attempt" == "30" ]]; then
    "${compose[@]}" logs app
    exit 1
  fi
  sleep 1
done

curl -fsS \
  -c "$cookie_file" \
  -H "Content-Type: application/json" \
  -d "{\"username\":\"$username\",\"password\":\"smoke-password\"}" \
  http://127.0.0.1:8080/login >/dev/null

invalid_image_status="$(
  curl -sS \
    -o "$tmp_dir/invalid-image.body" \
    -w "%{http_code}" \
    -b "$cookie_file" \
    -H "Content-Type: application/json" \
    -d '{"filename":"invalid.png","image":"aW52YWxpZA=="}' \
    http://127.0.0.1:8080/upload/send
)"
[[ "$invalid_image_status" == "400" ]]

oversized_image_status="$(
  curl -sS \
    -o "$tmp_dir/oversized-image.body" \
    -w "%{http_code}" \
    -b "$cookie_file" \
    -H "Content-Type: application/json" \
    -d '{"filename":"oversized.png","image":"iVBORw0KGgoAAAANSUhEUgABhqAAAYag"}' \
    http://127.0.0.1:8080/upload/send
)"
[[ "$oversized_image_status" == "400" ]]
grep -q "dimensions exceed" "$tmp_dir/oversized-image.body"

valid_image_status="$(
  curl -sS \
    -o "$tmp_dir/valid-image.body" \
    -w "%{http_code}" \
    -b "$cookie_file" \
    -H "Content-Type: application/json" \
    -d '{"filename":"pixel.png","image":"iVBORw0KGgoAAAANSUhEUgAAAAIAAAACCAIAAAD91JpzAAAAEklEQVR4nGP4z8DAAMIM/4EAAB/uBfsL2WiLAAAAAElFTkSuQmCC"}' \
    http://127.0.0.1:8080/upload/send
)"
if [[ "$valid_image_status" != "200" ]]; then
  cat "$tmp_dir/valid-image.body"
  "${compose[@]}" logs app
  exit 1
fi
valid_image_response="$(cat "$tmp_dir/valid-image.body")"
python3 -c '
import json
import sys

result = json.load(sys.stdin)
assert result["class_name"]
assert 0.0 <= float(result["confidence"]) <= 1.0
' <<<"$valid_image_response"

echo "Smoke test passed"
