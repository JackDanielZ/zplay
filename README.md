# ZPLAY

Simple player using EFL with commands given via stdin

### Prerequisites

Just EFL is needed

### Installing

sh make.sh

## Checking it works

* Without pipe:

```
zplay filename
```

* With pipe (example with a piped name):

```
mkfifo pipe
cat pipe | zplay [optional filename]
```

In another terminal (dummy writer because STDIN will return unavailable resource if no writer is on the pipe):

```
cat > pipe
```

In another terminal (real writer):

```
echo "PAUSE" > pipe
echo "SHOW_PROGRESS" > pipe
...
```

