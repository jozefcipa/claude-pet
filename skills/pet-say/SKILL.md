---
name: pet-say
description: Show a short message in a speech bubble on the user's desk pet (the claude-pet ESP32 display). Use when the user asks to send, show, or say something on the pet, display, or desk buddy, or invokes /pet-say.
---

# Say something on the desk pet

Run the `say.sh` script that sits next to this file, in this skill's base directory:

```sh
"<skill base directory>/say.sh" "<message>" [duration_ms]
```

- Message: the text the user asked for, or a short line you write if they asked you to come up with one. The bubble fits 3 lines of 24 characters (about 70 characters); anything longer gets cut off, so keep it short.
- ASCII only. The display font has no emoji or accented letters, so swap them for plain text.
- `duration_ms` is optional. By default the bubble stays up for a time based on its length (1.5 to 7 seconds). Pass a longer value, e.g. `10000`, only if the user asks for it to stay up longer.
- The bot keeps doing whatever it was doing; this only adds the bubble.
- The pet is found at `claude-pet.local`; the `CLAUDE_PET_HOST` environment variable overrides it.
- Exit code 1 ("pet not reachable") means the device is off or not on the network. Say so in one line and don't retry.

Afterwards, confirm in one short line what you sent.
