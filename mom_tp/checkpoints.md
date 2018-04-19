## Checkpoints

**0.** Define user process' _mom_ API. 																			**DONE**

**1.** Implement _mom_ requester and sender locally (i.e. without sockets).											**DONE**

**2.** Create user process and locally interact with _mom_.															**DONE**

**3.** Create broker server to make simple interactions with user's machine's _mom_ via sockets (no forking).			**DONE**

**4.** Create DBMS worker and make it persist stuff.															**WIP**

**5.** Make server broker fork the handler, and make it interact with the DBMS worker.

**6.** Make handler fork the sender, and make DBMS interact with it.