# concurrent_autocomplete
Autocomplete functionality implemented with threads. User inputs prefix, and gets back words that start with given prefix.
Potential words are in .txt files, and they are in trie structure which is searched concurrently. Words are added concurrently, aswell.
Command '_add_ <dir_name>' adds directory with given name, and makes a thread that is searching through files of that directory without stopping.
When searching the prefix, it writes out words with the prefix, and additionaly writes more words with that prefix as they are added,
until Ctrl+D is pressed.
Mutex locks the first trie node that has the same value as the first letter of the prefix.
