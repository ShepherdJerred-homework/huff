//huff.cpp
//By Jerred Shepherd and Mack Peters
//This program compresses a file using the huffman algorithm. The file can later be decompressed
//by the corresponding puff program.


#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <algorithm>
#include <string>
#include <iomanip>
#include <ctime>

using std::map;
using std::vector;
using std::string;

using std::ofstream;
using std::ifstream;
using std::ios;

using std::cin;
using std::cout;
using std::endl;

struct HuffTableEntry {
    int glyph = -1;
    int frequency = 0;
    int leftPointer = -1;
    int rightPointer = -1;
};

struct FileInfo {
    string fileName;
    int fileNameLength;
    ifstream fileStream;
    double fileStreamLength;
    int numberOfGlyphsInFile;
};

// Min heaping
// Marks current position as smallest, gets left and right children. If left child is not outside the heap
// compares its frequency to the smallest frequency. If it is smaller, set left child to the smallest. Then does the same
// with right child. If smallest is not equal to the original position fed in, switch the values for current position and the
// smallest value. Then re-check (which may be unnecessary...I can't remember why that's there)
void minHeap(vector<HuffTableEntry> &fileInfo, int position, int heapSize) {
    int smallest = position;
    int left = 2 * position + 1;
    int right = 2 * position + 2;

    if (left <= heapSize && fileInfo[left].frequency < fileInfo[smallest].frequency) {
        smallest = left;
    }

    if (right <= heapSize && fileInfo[right].frequency < fileInfo[smallest].frequency) {
        smallest = right;
    }

    if (smallest != position) {
        HuffTableEntry temp = fileInfo[position];
        fileInfo[position] = fileInfo[smallest];
        fileInfo[smallest] = temp;
        minHeap(fileInfo, smallest, heapSize);
    }
}

//Recursive function that generates byte codes. If the table entry is a merge node (-1 as glyph), we then look for
//left and right pointers. If the pointer goes left, add a 0 to the byte code, if it goes right, add a 1. Once we hit
//a leaf node (glyph not -1) then attach the string to that glyph. Stored in a map of <int, string> where the int is the glyph
//and the string is the bytecode. Ints are necessary because 256 is EOF and cannot be represented as a char.
void
generateByteCodes(vector<HuffTableEntry> &treeValues, map<int, string> &byteCodes, int position, string byteCode) {
    if (treeValues[position].leftPointer != -1 || treeValues[position].rightPointer != -1) {
        if (treeValues[position].leftPointer != -1) {
            generateByteCodes(treeValues, byteCodes, treeValues[position].leftPointer, (byteCode + "0"));
        }
        if (treeValues[position].rightPointer != -1) {
            generateByteCodes(treeValues, byteCodes, treeValues[position].rightPointer, (byteCode + "1"));
        }
    } else {
        byteCodes[treeValues[position].glyph] = byteCode;
    }
}

// Sort hufTable by frequency
bool sortByFrequency(HuffTableEntry &lhs, HuffTableEntry &rhs) {
    return lhs.frequency < rhs.frequency;
}

//Gets file contents and also determines how big the file is.
void loadFileContents(FileInfo &fileInfo) {
    fileInfo.fileStream = ifstream(fileInfo.fileName, ios::in | ios::binary);

    // Find how long the file is, store that number, and return to the beginning of the file
    fileInfo.fileStream.seekg(0, fileInfo.fileStream.end);
    fileInfo.fileStreamLength = fileInfo.fileStream.tellg();
    fileInfo.fileStream.seekg(0, fileInfo.fileStream.beg);
}

//Reads through the file and stores how often a glyph appears.
//Glyphs and values are stored in a map of <int,int>.
map<int, int> getGlyphFrequencies(FileInfo &fileInfo) {
    map<int, int> glyphFrequencies;

    // Putting values into a map and incrementing repeat offenders
    for (int i = 0; i < fileInfo.fileStreamLength; i++) {
        int num = 0;
        fileInfo.fileStream.read((char *)&num, 1);
        fileInfo.fileStream.seekg(0, 1);

        glyphFrequencies[num]++;
    }

    // Adding the eof character
    glyphFrequencies[256] = 1;

	//return to beginning of file for later
	fileInfo.fileStream.seekg(0, ios::beg);

    return glyphFrequencies;
}


// TODO find better fileName
//Creates a vector of HuffTableEntry to support creating a huffman table later on. Table must be
//the number of glpyhs in file plus number of glyphs in file minus one to support
//the huffman algorithm. Vector of correct size is created then we iterate through the map
//of glyphs and frequencies and add those values to the slots in the array. It then sorts the array from
//smallest to largest to allow the huffman algorithm to work in a later step.
vector<HuffTableEntry> createSortedVectorFromFile(FileInfo &fileInfo) {
    map<int, int> glyphFrequencies = getGlyphFrequencies(fileInfo);

    fileInfo.numberOfGlyphsInFile = glyphFrequencies.size();

    // Creates a vector that's as big as we need so that it can be sorted by value
    vector<HuffTableEntry> huffTableVector(fileInfo.numberOfGlyphsInFile + (fileInfo.numberOfGlyphsInFile - 1));

    // Put map into vector
    int arrayLocation = 0;
    for (auto entry : glyphFrequencies) {
        huffTableVector[arrayLocation].glyph = entry.first;
        huffTableVector[arrayLocation].frequency = entry.second;
        arrayLocation++;
    }

    sort(huffTableVector.begin(), huffTableVector.begin() + fileInfo.numberOfGlyphsInFile, sortByFrequency);

    return huffTableVector;
}

//Fills in the huffman table by following steps learned in class. We first mark the end of the heap
//and the first free slot. Then we mark which of slots 1 and 2 in the vector have the lowest value. We 
//move the marked lowest value to the first free slot, then move the end of the heap to the empty slot just
//create by moving the lowest value. We then reheap to ensure that we maintain a min heap. Then we move the 
//value in slot 0 to the end of the heap, and in slot 0 we create a merge node containing the value just moved
//from the front of the heap as the left pointer and the value in the first free slot as the right pointer. We
//reheap once more and then move the end of the heap to the left as to not include the node that was just moved
//and the first free slot to the right to the next free slot.
vector<HuffTableEntry> createHuffmanTable(FileInfo &fileInfo) {
    vector<HuffTableEntry> huffTable = createSortedVectorFromFile(fileInfo);

    // Creating the full huffman table
    int heapEnd = fileInfo.numberOfGlyphsInFile - 1;
    int firstFreeSlot = fileInfo.numberOfGlyphsInFile;
    int marked;

    // Stops 2 early because the last merge doesn't need to be this intense
    for (int i = 0; i < (fileInfo.numberOfGlyphsInFile - 2); i++) {
        marked = (huffTable[1].frequency < huffTable[2].frequency) ? 1 : 2;
        huffTable[firstFreeSlot] = huffTable[marked];
        huffTable[marked] = huffTable[heapEnd];

        for (int k = (heapEnd - 1) / 2; k >= 0; k--) {
            minHeap(huffTable, k, heapEnd - 1);
        }

        huffTable[heapEnd] = huffTable[0];
        huffTable[0].glyph = -1;
        huffTable[0].frequency = huffTable[heapEnd].frequency + huffTable[firstFreeSlot].frequency;
        huffTable[0].leftPointer = heapEnd;
        huffTable[0].rightPointer = firstFreeSlot;

        for (int l = (heapEnd - 1) / 2; l >= 0; l--) {
            minHeap(huffTable, l, heapEnd - 1);
        }

        heapEnd--;
        firstFreeSlot++;
    }

    // Last merge
    huffTable[firstFreeSlot] = huffTable[0];
    huffTable[0].glyph = -1;
    huffTable[0].frequency = huffTable[1].frequency + huffTable[firstFreeSlot].frequency;
    huffTable[0].leftPointer = 1;
    huffTable[0].rightPointer = firstFreeSlot;

    return huffTable;
}

//Acts as an entry point to generate the byte codes. Implemented in order to keep other functions more organized
map<int, string> generateByteCodeTable(vector<HuffTableEntry> &huffTable) {
    map<int, string> byteCodes;
    string byteCode;

    generateByteCodes(huffTable, byteCodes, 0, byteCode);

    return byteCodes;
}

/**
 * Reads through every byte in a file, and encodes it using a map
 * @param fileInfo The fileInfo object to use; contains the bytes to be encoded
 * @param map The map to encode with
 * @return An encoded string of 0's and 1's
 */
string encodeMessageToStringOfBits(FileInfo &fileInfo, map<int, string> &map) {

    string s = "";
    for (int i = 0; i < fileInfo.fileStreamLength; i++) {
        int num = 0;
        fileInfo.fileStream.read((char *)&num, 1);
        fileInfo.fileStream.seekg(0, 1);

		s += map[num];
    }

    // Add the eof character
	s += map[256];

    return s;
}

/**
 * Encodes a single byte
 * @param byte A string of 8 bits
 * @return A byte
 */
unsigned char encodeByte(string byte) {
    // byte to be encoded
    unsigned char byte1 = '\0';
    // length of huffman code
    int bitstringLength = byte.length();

    // building an encoded byte from right to left
    int cnt = 0;
    for (int i = 0; i < bitstringLength; i++) {
        // is the bit "on"?
        if (byte[i] == '1')
            // turn the bit on using the OR bitwise operator
            byte1 = byte1 | (int) pow(2.0, cnt);
        cnt++;
    }
    return byte1;
}

/**
 * Takes a string of bits and converts it to a string of bytes
 * @param message A string of bits
 */
string encodeMessageToStringOfBytes(string &message) {
    string bytes;
    for (unsigned i = 0; i < message.length(); i += 8) {

        string newMessage = message.substr(i, 8);
        if (newMessage.length() < 8 && newMessage.length() > 0) {
            int numberNeeded = 8 - newMessage.length();
            for (int x = 0; x < numberNeeded; x++) {
                newMessage += "0";
            }
        }

		bytes += encodeByte(newMessage);
    }

    return bytes;
}

//Creates the .huf version of the file.
//First writes out the file name length, the file name itself, and then the number of table entries. It then
//loops through the huffman table and prints out each glyph, left and right pointer in each slot. Lastly, it
//writes out the entire compressed message.
void createAndOutputFileInfo(FileInfo &fileInfo, vector<HuffTableEntry> &huffTableEntries, string &bytes) {
    // Strips away any extension from filename. If there isn't one, then just creates
    // a copy of the original filename.
    int pos = fileInfo.fileName.find_last_of(".");
    string fileNameWithoutExtension = fileInfo.fileName.substr(0, pos);

    ofstream fout(fileNameWithoutExtension + ".huf", ios::out | ios::binary);

    int numberOfTableEntries = huffTableEntries.size();

    fout.write((char *) &fileInfo.fileNameLength, sizeof fileInfo.fileNameLength);
    fout.write(fileInfo.fileName.c_str(), fileInfo.fileName.size());
    fout.write((char *) &numberOfTableEntries, sizeof numberOfTableEntries);

    for (int i = 0; i < numberOfTableEntries; i++) {
        fout.write((char *) &huffTableEntries[i].glyph, sizeof huffTableEntries[i].glyph);
        fout.write((char *) &huffTableEntries[i].leftPointer, sizeof huffTableEntries[i].leftPointer);
        fout.write((char *) &huffTableEntries[i].rightPointer, sizeof huffTableEntries[i].rightPointer);
    }

    fout.write(bytes.c_str(), bytes.size());

    fout.close();

}

void main() {

    string fileName;
    cout << "Enter the fileName of a file to be read: ";
    getline(cin, fileName);

	clock_t start, end;
	start = clock();

    FileInfo fileInfo;
    fileInfo.fileName = fileName;
    fileInfo.fileNameLength = fileName.length();

    loadFileContents(fileInfo);
    vector<HuffTableEntry> huffTable = createHuffmanTable(fileInfo);
    map<int, string> encodingMap = generateByteCodeTable(huffTable);

    string message = encodeMessageToStringOfBits(fileInfo, encodingMap);

    string bytes = encodeMessageToStringOfBytes(message);

    createAndOutputFileInfo(fileInfo, huffTable, bytes);

    fileInfo.fileStream.close();

	end = clock();
	cout << std::setprecision(1) << std::fixed;
	cout << "The time was " << (double(end - start) / CLOCKS_PER_SEC) << " seconds." << endl;

}