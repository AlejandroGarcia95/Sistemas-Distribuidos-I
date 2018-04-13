## Checkpoints

**0.** Define user process' _mom_ API.

**1.** Implement _mom_ requester and sender locally (i.e. without sockets).

**2.** Create user process and locally interact with _mom_.

**3.** Create broker server to make simple interactions with user's machine's _mom_ via sockets (no forking).

**4.** Create DBMS worker and make it persist stuff.

**5.** Make server broker fork the handler, and make it interact with the DBMS worker.

**6.** Make handler fork the sender, and make DBMS interact with it.