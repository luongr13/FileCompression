//
// STARTER CODE: util.h
//
// Edited by: Richard Luong
// UIC, Spring 2022
// CS251, Data Structures
//
// Description:
// This file includes the implementation to compress a text file using
// Huffman's compression algorithm using a hashmap with chaining to handle
// collisions for the frequency map and a binary tree for encoding and decoding.
//
#include <iostream>
#include <fstream>
#include <map>
#include <queue>          // std::priority_queue
#include <vector>         // std::vector
#include <functional>     // std::greater
#include <utility>
#include <string>
#include "bitstream.h"
#include "hashmap.h"
#include "mymap.h"
#pragma once

struct HuffmanNode {
    int character;
    int count;
    HuffmanNode* zero;
    HuffmanNode* one;
};
//
// functor for priority_queue
//
class prioritize {
	public: bool operator() (const HuffmanNode* p1, const HuffmanNode* p2) const {
		return p1->count > p2->count;
	}
};

// freeTree
//
// Deallocates memory associated with the tree for encoding/decoding
//
void freeTree(HuffmanNode* node) {
    if (node == nullptr)
		return;
	freeTree(node->zero);
	freeTree(node->one);

	delete(node);
}

// putFrequency
//
// Updates the value if a key already exists in the hash map.
// Otherwise, add a new entry.
//
void putFrequency(hashmap &map, int c) {
	int count;

	if (map.containsKey(c)) {		// key exists, update frequency count
		count = map.get(c);
		map.put(c, count+1);
	} else {							// key doesnt exist, simply put new key
		map.put(c, 1);
	}
}

// buildFrequencyMap
//
// Builds a frequency map using data from a file if ifFile is true.
// If not true, using the string from filename to build a frequency map.
//
void buildFrequencyMap(string filename, bool isFile, hashmap &map) {
	char c = 0;
	if (isFile) {
		ifstream infile(filename);
		while (infile.get(c))
			putFrequency(map, c);
	} else {
		for (char c : filename)
			putFrequency(map, c);
	}

	putFrequency(map, PSEUDO_EOF);
}

// buildEncodingTree
//
// Creates an encoding tree from a given hashmap using a priority queue.
// Characters with smaller frequencies have a higher priority in the queue.
//
HuffmanNode* buildEncodingTree(hashmap &map) {
    priority_queue<HuffmanNode*, vector<HuffmanNode*>, prioritize> pq;

	vector<int> keys = map.keys();

	// create a HuffmanNode for each map entry and
	// push into priority queue
	for (int x : keys) {
		HuffmanNode* newN = new HuffmanNode;
		newN->character = x;
		newN->count = map.get(x);
		newN->zero = nullptr;
		newN->one = nullptr;
		pq.push(newN);
	}

	// build tree
	while (pq.size() != 1) {
		HuffmanNode* leftN = pq.top();
		pq.pop();
		HuffmanNode* rightN = pq.top();
		pq.pop();

		HuffmanNode* parent = new HuffmanNode;
		parent->character = NOT_A_CHAR;
		parent->count = leftN->count + rightN->count;
		parent->zero = leftN;
		parent->one = rightN;

		pq.push(parent);
	}

	HuffmanNode* root = pq.top();
	pq.pop();

    return root;
}

// buildBinary
//
// Helper function for buildEncodingMap to recursively build a binary
// representation in string format.
//
void buildBinary(string str, HuffmanNode* node, mymap<int, string>& map) {
	// base case
	if (node->character != NOT_A_CHAR) {
		map.put(node->character, str);
		return;
	}
	// recursive calls
	buildBinary(str+"0", node->zero, map);
	buildBinary(str+"1", node->one, map);
}

// buildEncodingMap
//
// Given an encoded tree, builds an encoding map using mymap.
//
mymap <int, string> buildEncodingMap(HuffmanNode* tree) {
    mymap <int, string> encodingMap;
	buildBinary("", tree, encodingMap);
    return encodingMap;
}

// encode
//
// Reads the data from the input stream, creates a string from the encoding map,
// and writes to the output stream if makeFile is true.
// The functions returns the # of bits written by reference and
// returns the encoding in string format for testing purposes by value.
//
string encode(ifstream& input, mymap <int, string> &encodingMap,
              ofbitstream& output, int &size, bool makeFile) {
	string str = "";
	char c;

	// encode data from input stream
	while (input.get(c)) {
		str += encodingMap.get(c);
	}
	str += encodingMap.get(PSEUDO_EOF);

	// # of bits written to output
	size = str.size();

	// write to output stream if true
	if (makeFile) {
		for(char x : str)
			output.writeBit(x - '0');
	}

    return str;
}

// decode
//
// Reads data from the input stream, decodes the data using the encoding tree,
// and writes the decoded data to the output stream. The decoded data is
// returned as a string for testing purposes.
//
string decode(ifbitstream &input, HuffmanNode* encodingTree, ofstream &output) {
	HuffmanNode* cur = encodingTree;
    string decoded;

    while (!input.eof()) {
		// break out of loop if end of file is reached
		if (cur->character == PSEUDO_EOF)
			break;
		// if we are at leaf node...
		// 1. append to decoded string
		// 2. write to output stream
		// 3. start a root node again
		if (cur->character != NOT_A_CHAR) {
			decoded += cur->character;
			output.put(cur->character);
			cur = encodingTree;
		}
		// decode input stream to determine where to traverse
        int bit = input.readBit();
		if (bit == 0) {
			cur = cur->zero;
		} else if (bit == 1) {
			cur = cur->one;
		}
    }

    return decoded;
}

// compress
//
// This function completes the entire compression process.  Given a file,
// filename, this function (1) builds a frequency map; (2) builds an encoding
// tree; (3) builds an encoding map; (4) encodes the file (don't forget to
// include the frequency map in the header of the output file).  This function
// should create a compressed file named (filename + ".huf") and should also
// return a string version of the bit pattern.
//
string compress(string filename) {
	// 1. build frequency map
	hashmap frequencyMap;
	buildFrequencyMap(filename, true, frequencyMap);

	// 2. build encoding tree
	HuffmanNode* tree = buildEncodingTree(frequencyMap);

	// 3. build encoding map
	mymap<int, string> encodingMap = buildEncodingMap(tree);

	// 4. Set up output stream for compressed file
	//	  and inputstream to read data
	ofbitstream output(filename + ".huf");
	ifstream input(filename);
	output << frequencyMap;

	// 5. encode data and create string to return
	int size = 0;
	string codeStr = encode(input, encodingMap, output, size, true);

    output.close();
	freeTree(tree);

    return codeStr;
}

// decompress
//
// This function completes the entire decompression process.  Given the file,
// filename (which should end with ".huf"), (1) extract the header and build
// the frequency map; (2) build an encoding tree from the frequency map; (3)
// using the encoding tree to decode the file.  This function should create a
// compressed file using the following convention.
// If filename = "example.txt.huf", then the uncompressed file should be named
// "example_unc.txt".  The function should return a string version of the
// uncompressed file.  Note: this function should reverse what the compress
// function did.
//
string decompress(string filename) {
	// 1a. extract header
    size_t pos = filename.find(".huf");
	if ((int)pos >= 0)
		filename = filename.substr(0, pos);
	pos = filename.find(".");

    string ext = filename.substr(pos, filename.length() - pos);
    filename = filename.substr(0, pos);

    ifbitstream input(filename + ext + ".huf");
    ofstream output(filename + "_unc" + ext);

	// 1b. build frequency map;
    hashmap dump;
    input >> dump;

	// 2. build encoding tree
	HuffmanNode* encodingTree = buildEncodingTree(dump);

	// 3. decode file and store as a string
    string decodeStr = decode(input, encodingTree, output);

    output.close();
	freeTree(encodingTree);

    return decodeStr;
}
