# QuizGame app

The application consists of a **multi-threaded** server that can support an **unlimited** number of clients. It is an interactive way of testing people, so it can be used for educational purposes and is very useful in simulation or knowledge testing contexts. The main goal is to create a server that allows connecting an unlimited number of clients simultaneously so that the server uses as few resources as possible.

## Used Technologies
* The application uses a **TCP** protocol because it ensures the correct transmission of data between the source and the recipient, thus any quiz participant will be validated based on his answer, not possibly that of another participant.
* **Multithreading** to support multiple connections
simultaneous . The number of customers who can participate in a quiz
is unlimited, which is ensured by the implementation of a **Thread Pool**.
* **Asynchronous multiplexing** to process the requests simultaneously, without waiting for the completion of one request before starting another and to impose a certain response time for each question in the quiz.
* **JSON format**, the application stores the details of the questions of each quiz in a json file



## How the Thread Pool works
Each **thread** can execute several requests, that is, it can work for several clients (participants) iteratively. After finishing a client's request, it will move on to the next available client.The number of **threads** used by the application increases depending on how many requests must be executed.
### Implementation
![ThreadPool class][./pictures/thread_pool.png]

## Application Level Protocol
* The client will wait for the quiz to start
* He will choose if he wants to participate in the quiz 
* He will choose a nickname
* The server will send questions to each connected client and validate the answers
* The server will send each client the final ranking

##  Installation
### Prerequisites
g++
### Steps
* Clone the repository
```bash
git clone https://github.com/tudorp200/quizGame

```
* Build the project
```bash
make
```
* Make a quiz in a json file and than move it to **/tmp/quizzes/quiz1.json** 

## How to use it
### Steps
* Run the server
```bash
./server
```
* Run a client or more
```bash
./client <server_adress> <port>
```
#### If you want to run it locally:
```bash
./client 0 2020
```

## Example of use
![clients][./pictures/quiz2.png]
![server][./pictures/quiz1.png]
