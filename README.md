# chatbot-c

A rule-based chatbot written in pure C. No dependencies, no internet required —
just compile and run.

## Features

- Pattern-matched responses (greetings, math, time, jokes, help topics, ...)
- Simple arithmetic evaluation (`calc 12 * (3 + 4)`)
- Remembers your name during the session
- Fuzzy fallback with keyword scoring when no exact rule matches
- Exit commands: `bye`, `quit`, `exit`

## Build

Requires GCC (or any C99 compiler).

```sh
gcc -std=c99 -Wall -Wextra -O2 -o chatbot src/chatbot.c
```

Or with make:

```sh
make
```

## Run

```sh
./chatbot        # Linux / macOS
chatbot.exe      # Windows
```

## Example session

```
Bot: Hi! I'm Chip. Type 'help' to see what I can do.
You: hello
Bot: Hey there! What's your name?
You: Sam
Bot: Nice to meet you, Sam!
You: calc 12 * (3 + 4)
Bot: 12 * (3 + 4) = 84
You: tell me a joke
Bot: Why do C programmers never get Halloween mixed up with Christmas?
     Because OCT 31 == DEC 25.
You: bye
Bot: Goodbye, Sam! Come back soon.
```

## Project structure

```
chatbot-c/
├── src/chatbot.c    # all the bot logic
├── Makefile
└── README.md
```

## License

MIT
