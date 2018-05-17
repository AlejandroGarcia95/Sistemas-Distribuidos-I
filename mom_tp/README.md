# MOM TP1 for Sistemas Distribuidos I (FIUBA)

## Compiling

​	For compiling the whole TP, simply run the `make` command on this `mom_tp` directory. Several executable files will be generated, including:

- _broker_server_: The broker process that will be the middleware between many users. It's main purpose is accepting incoming connections from user's machines. This process must be launched before any other in the broker machine.
- _broker_handler_ and _broker_sender_: This duo handles requests from users, and send the responses to them. The _broker_server_ automatically launches these two processes every time a new machine connects to the MOM.
- _dbms_: The database manager system process. This process controls the entire user and topic's DB. Internally it launches a new worker for answering every request, locking the DB as needed. Launched by _broker_server_.
- _mom_daemon_: The main process to be launched at every user machine. Only one instance of this daemon should be running at a time. 
- _mom_requester_ and _mom_responser_: This pair, automatically launched by the _mom_daemon_ on the user's machines, are in charge of sending requests to and receiving responses from the broker. 
- _mom_forwarder_: This process receives from _mom_responser_ all those messages intended to be delivered after some user performs a _publish_ call (i.e. messages that should be received when calling _receive_, not the broker ACKs), and persists them. As such, when a user process performs a _receive_ call, this process gets notificated and pushes the correct message in the System V queue, _preventing it from getting full_ . Added on my own because I saw it was good.

## Launching broker

​	For launching the broker, just type `./broker_server IP PORT` , where `IP` matches the address of your broker machine (e.g. 192.168.0.10) and `PORT` matches the port to run the server broker (e.g. 8088). If you plan to **test things locally**, simply typing `make run_boker` will launch the _broker_server_ with default IP localhost address and port 8080.

​	Note: For stopping the _broker_server_, hit `CTRL+C`.

## Launching daemon

​	After launching the broker, you can launch the _mom_daemon_ on the user machine by typing `./mom_daemon IP PORT` , where `IP` matches the address of your broker machine and `PORT` matches the port to run the server broker as explained above. If you plan to **test things locally**, simply typing `make run_daemon` will launch the *mom_daemon* with default IP localhost address and port 8080.

​	Note: For stopping the _mom_daemon_, hit `CTRL+C`.

## Testing something!

​	An executable _user_main_ file is also including to test the MOM behaivour. By running it with `./user_main` , you will be provided a little in-screen help for performing the MOM API calls. You could, let's say, launch several instances of this program, subscribe all of them to some random topics, and publish-receive to exchange messages.



## Documentation, diagrams and experimental stuff	

- The sequence diagrams are includded on a _.zip_ file on this directory.
- All of the complex parts of the code (IPC implementations, the database manager system, and some other dark magic functions) are properly documented with comments.
- A Python version of the API is provided in the _mom.py_ file (requires the module _sysv_ipc_ to be installed in your system). Unfortunately, it has only been proved to be properly working on 32 bits Ubuntu.
- The museum excercise has been implemented, with a distributed version of a resources coordinator (handling the semaphores and shared memories). However, this coordinator relies on the Python API.
- Another simulation featuring the producer-consumer philosophers problem has also been implemented, with a better version of the resources coordinator (with fault tolerance and resource sharding). However, it also relies on the Python API.