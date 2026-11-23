#!/usr/bin/env bash
# SMARTLUX project guardrail — Claude Code PreToolUse hook.
# Reads the hook JSON payload on stdin, exits 2 (block) with a reason on stderr
# when an action would violate a binding rule from AGENTS.md.
# Dependency-free: bash + grep only, so it also works on WSL/CI images.

set -uo pipefail

payload=$(cat)

block() {
  printf 'SMARTLUX guardrail: %s\n' "$1" >&2
  exit 2
}

# Normalize once: the payload is JSON, we only need coarse substring matching.
flat=$(printf '%s' "$payload" | tr '\n' ' ')

proposal='proposal_SMARTLUX_Kelompok_2.md'

# 1. Never overwrite the proposal wholesale. Two ~200 KB lines embed base64
#    PNGs; a full-file write or a formatter destroys them. Targeted edits only.
if printf '%s' "$flat" | grep -q '"tool_name"[[:space:]]*:[[:space:]]*"Write"'; then
  if printf '%s' "$flat" | grep -qF "$proposal"; then
    block "do not Write/replace $proposal wholesale. It embeds two base64 PNG lines (~513 KB total). Use targeted line-based edits instead."
  fi
fi

# 2. No formatters/rewriters over the proposal.
if printf '%s' "$flat" | grep -qF "$proposal"; then
  if printf '%s' "$flat" | grep -Eq 'prettier|markdownlint|mdformat|sed -i|perl -pi|dos2unix|unix2dos|tr -d'; then
    block "no formatter or in-place rewriter may touch $proposal (base64 image lines would be corrupted)."
  fi
fi

# 3. Secrets must never be staged or committed. secrets.example.h is fine.
if printf '%s' "$flat" | grep -Eq 'git (add|commit|stage)'; then
  if printf '%s' "$flat" | grep -Eq 'secrets\.h|\.env[^a-zA-Z]|\.env$'; then
    if ! printf '%s' "$flat" | grep -q 'secrets\.example\.h'; then
      block "never stage secrets.h or .env files. WiFi credentials live in the git-ignored firmware/SMARTLUX/secrets.h; commit secrets.example.h only."
    fi
  fi
fi

# 4. Destructive git operations on a repo with no upstream safety net.
if printf '%s' "$flat" | grep -Eq 'git push[^"]*(--force|[[:space:]]-f([[:space:]]|$))|git reset --hard|git clean -[a-z]*f|git branch -D'; then
  block "destructive git command blocked (force-push, reset --hard, clean -f, branch -D). Ask the group first; main is protected but local history is not recoverable."
fi

# 5. Do not commit rendered enclosure binaries.
if printf '%s' "$flat" | grep -Eq 'git (add|commit)' && printf '%s' "$flat" | grep -Eq '\.stl'; then
  block "do not commit *.stl binaries. The enclosure ships as parametric smartlux_case.scad; STL output is git-ignored."
fi

exit 0
