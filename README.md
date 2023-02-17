# Operating System Course Projects

- [Operating System Course Projects](#operating-system-course-projects)
  - [CA1: Socket Programming](#ca1-socket-programming)
  - [CA2: MapReduce Counter](#ca2-mapreduce-counter)
  - [CA3: Parallel Image Processing](#ca3-parallel-image-processing)

## CA1: Socket Programming

A buyer-seller auction program is implemented in C using the POSIX sockets API.  
There can be many processes, and each take a port to listen for UDP broadcasts of product changes:

```text
./buyer <PORT>
./seller <PORT>
```

The seller can add new products, chat with multiple buyers, and accept or reject their offers.  
The buyer has a list of the available products from all of the sellers on the same broadcast port, and can message the sellers and offer to buy their product.  
The CLI commands can be listed using the `help` command.

Internally, messaging is implemented using TCP sockets. They can begin with different commands such as `$MSG$` for normal chats, `$OFR$` when the buyer offers a price, and the seller lets the buyer know whether the offer was `$ACC$` or `$REJ$` before it is disconnected.  
The buyers are informed of all changes and the seller broadcasts are logged in the CLI.

## CA2: MapReduce Counter

A MapReduce model is used to count the number of books in each genre listed in CSV files.  
The main program takes the path of a folder containing `genres.csv`, a CSV file listing all of the possible genres, and some `part<N>.csv` files which list books with their genres next to them.

```text
./GenreCounter.out <LIBRARY PATH>
```

The program will read `genres.csv`, create a reducer process *(reduce.cpp)* for each genre, and create a mapper process *(map.cpp)* for each `part<N>.csv` file.  
Each mapper will read a `part<N>.csv` file and count the number of books in each genre of the file. This is then passed to the reducer of each genre and they calculate the sum and print it.

The mappers need the available genres as to ignore new genres in the part files. This is passed by the main process to each mapper using POSIX pipes.  
To receive the count of genres in each file, each reducer makes its own named pipe (FIFO) named after the genre it is counting.

## CA3: Parallel Image Processing

Image filters and effects are applied to a *BMP24* image file both serially, and in parallel using POSIX threads.  
The program take an image as input, reads it using the fully implemented `Bmp` class *(bmp.hpp)*, and applies the wanted filters which are implemented in the `filter` namespace *(filter.hpp)*.

```text
./ImageFilters.out <IMAGE FILE>
```

Each filter takes a `BmpView`, which is a custom view of the original `Bmp` (this can be the whole image as well which is done implicitly), and changes the original image pixels.  

In the example *main.cpp*, three filters (horizontal flip, emboss, and diamond) are applied and the result is written in `output.bmp`.

For the parallel version, a thread pool is implemented using pthreads.  
Tasks are added to a mutex protected queue which threads execute.  
In the example *main.cpp*, the image is split into 8 BmpViews (one for each thread) and the filter tasks are ran by the threads concurrently.
