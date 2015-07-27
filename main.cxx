#include <algorithm>
#include <map>
#include <cstdio>
#include <string>
#include <thread>


// Data structure to hold words
struct Word
{
	std::string data;

	Word ( char * data_ ) :data(data_) {}
	
	Word () {}
};

// Data structure to hold individual word count
struct wordCount
{
	int count;
	
	wordCount () :count(0) {}
};

// Choosing maps over vectors for the following reasons:
// 1. Sorted order is important
// 2. No duplicates allowed
// 3. Entire data struct does not need to be traversed during lookup 
static std::map<std::string, wordCount> s_wordsMap;
static std::mutex s_mutex;
static std::condition_variable s_conditionVariable;
static Word s_word;
static int s_totalFound;

// Worker thread: consume words passed from the main thread and insert them
// in the 'word list' (s_wordsMap). Terminate when the word 'end' is encountered.
static void worker_thread ()
{
	bool endEncountered = false;
    
	while (!endEncountered)
	{
		Word w;
		{
			// Take ownership of mutex for the duration of this scoped block
			std::lock_guard<std::mutex> lock(s_mutex);
			// Swap word struct with empty struct
			std::swap(w, s_word);
			// Unblock all threads waiting for this condition variable
			s_conditionVariable.notify_one();
		}
	  
		if (!w.data.empty())
		{
			endEncountered = std::strcmp( w.data.c_str(), "end" ) == 0;
      
			if (!endEncountered)
			{
				// Do not insert duplicate words
				auto& got = s_wordsMap[w.data];
				got.count += 1;
			}
		}
	}
};

// Read input words from STDIN and pass them to the worker thread for
// inclusion in the word list.
// Terminate when the word 'end' has been entered.
//
static void read_input_words ()
{
	bool endEncountered	= false;
	char lineBuf[32]	= {0};
	
	std::thread worker( worker_thread );
	while (!endEncountered)
	{
		if (std::scanf( "%s", lineBuf ) == EOF) // EOF?
		{
			return;
		}
		
		endEncountered = std::strcmp( lineBuf, "end" ) == 0;
	  
		// Pass the word to the worker thread
		{
			// Take ownership of mutex for the duration of this scoped block
			std::lock_guard<std::mutex> lock(s_mutex);
			s_word = Word(lineBuf);
		}
		s_conditionVariable.notify_one();
		
		// Wait for the worker
		{
			// Synchronize data access by the threads, making one thread wait for the content to be stored before its read by the second thread
			std::unique_lock<std::mutex> ulock(s_mutex);
			
			// Block current thread until the condition variable is woken up and check if the struct is empty i.e. data element has been read by the other thread
			s_conditionVariable.wait(ulock, []{ return s_word.data.empty(); });
		}
	}

	worker.join(); // Wait for the worker to terminate
}

// Repeatedly ask the user for a word and check whether it was present in the word list
// Terminate on EOF or the word 'end' has been entered
//
static void lookup_words ()
{
	bool found			= false;
	char lineBuf[32]	= {0};

	for(;;)
	{
		found = false;
		std::printf( "\nEnter a word for lookup:" );
		if (std::scanf( "%s", lineBuf ) == EOF)
		{
			return;
		}
		
		// Initialize the word to be searched
		Word w(lineBuf);

		// Search for the word
		auto result = s_wordsMap.find (w.data);
		// Check if word is found
		if ( result != s_wordsMap.end() )
		{
			found = true;
		}
		
		if (found)
		{
			std::printf( "SUCCESS: '%s' was present %d times in the initial word list\n",
						result->first.c_str(), result->second.count );
			++s_totalFound;
		}
		else
		{
			std::printf( "'%s' was NOT found in the initial word list\n", w.data.c_str() );
		}
	}
}

int main ()
{
	try
	{
		read_input_words();

		// Sort the words alphabetically : Data in Map is sorted per key.
		// Print the word list
		std::printf( "\n=== Word list:\n" );
		for (auto i = s_wordsMap.begin(); i != s_wordsMap.end(); i++)
		{
			std::printf( "%s %d\n", i->first.c_str(), i->second.count );
		}
		
		lookup_words();
	  
		printf( "\n=== Total words found: %d\n", s_totalFound );
	}
	catch (std::exception & e)
	{
		std::printf( "error %s\n", e.what() );
	}
  
	return 0;
}

