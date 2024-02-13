```
➜  hw2-echo-shell ./client -p "Hello there server, how are you?"
HEADER VALUES 
  Proto Type:    PROTO_CS_FUN
  Proto Ver:     VERSION_1
  Command:       CMD_PING_PONG
  Direction:     DIR_RECV
  Term:          TERM_FALL 
  Course:        NONE
  Pkt Len:       29

RECV FROM SERVER -> PONG: Hello there serv
➜  hw2-echo-shell ./client
HEADER VALUES 
  Proto Type:    PROTO_CS_FUN
  Proto Ver:     VERSION_1
  Command:       CMD_CLASS_INFO
  Direction:     DIR_RECV
  Term:          TERM_FALL 
  Course:        CS472
  Pkt Len:       12

RECV FROM SERVER -> CS472: Welcome to computer networks
➜  hw2-echo-shell ./client -c cs577
HEADER VALUES 
  Proto Type:    PROTO_CS_FUN
  Proto Ver:     VERSION_1
  Command:       CMD_CLASS_INFO
  Direction:     DIR_RECV
  Term:          TERM_FALL 
  Course:        cs577
  Pkt Len:       12

RECV FROM SERVER -> CS577: Software architecture is important
➜  hw2-echo-shell ./client -c bad 
HEADER VALUES 
  Proto Type:    PROTO_CS_FUN
  Proto Ver:     VERSION_1
  Command:       CMD_CLASS_INFO
  Direction:     DIR_RECV
  Term:          TERM_FALL 
  Course:        bad
  Pkt Len:       12

RECV FROM SERVER -> Requested Course Not Found
```