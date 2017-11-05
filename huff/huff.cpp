#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <algorithm>
#include <string>
#include <iomanip>

using std::map;
using std::vector;
using std::string;

using std::ofstream;
using std::ifstream;
using std::ios;

using std::cin;
using std::cout;
using std::endl;

struct huffTableEntry {
    int glyph = -1;
    int frequency = 0;
    int leftPointer = -1;
    int rightPointer = -1;
};

struct fileInfo {
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
void minHeap(vector<huffTableEntry> &fileInfo, int position, int heapSize) {
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
        huffTableEntry temp = fileInfo[position];
        fileInfo[position] = fileInfo[smallest];
        fileInfo[smallest] = temp;
        minHeap(fileInfo, smallest, heapSize);
    }
}

void
generateByteCodes(vector<huffTableEntry> &treeValues, map<char, string> &byteCodes, int position, string byteCode) {
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
bool sortByFrequency(huffTableEntry &lhs, huffTableEntry &rhs) {
    return lhs.frequency < rhs.frequency;
}

void createAndOutputfileInfo(fileInfo fileInfoInfo, double length, vector<huffTableEntry> huffTableEntries) {
    // Strips away any extension from filename. If there isn't one, then just creates
    // a copy of the original filename.
    int pos = fileInfoInfo.fileName.find_last_of(".");
    string fileName = fileInfoInfo.fileName.substr(0, pos);

    string fileInfo = fileName + ".huf";

    ofstream fout(fileInfo, ios::out | ios::binary);

	fout.write((char*)&fileInfoInfo.fileNameLength, sizeof fileInfoInfo.fileNameLength);
	fout.write((char*)&fileInfo, fileInfo.length());
	fout.write((char*)&fileInfoInfo.numberOfGlyphsInFile, sizeof fileInfoInfo.numberOfGlyphsInFile);
	for (int i = 0; i < fileInfoInfo.numberOfGlyphsInFile; i++)
	{
		fout.write((char*)&huffTableEntries[i].glyph, sizeof huffTableEntries[i].glyph);
		fout.write((char*)&huffTableEntries[i].leftPointer, sizeof &huffTableEntries[i].leftPointer);
		fout.write((char*)&huffTableEntries[i].rightPointer, sizeof &huffTableEntries[i].rightPointer);
	}
	fout.write((char*)&fileInfoInfo.
}

int getAsciiValue(unsigned char c) {
    return (int) c;
}

// TODO close file
// TODO map file into memory (https://stackoverflow.com/questions/15138353/how-to-read-a-binary-file-into-a-vector-of-unsigned-chars)
void loadFileContents(fileInfo &fileInfo) {
    fileInfo.fileStream = ifstream(fileInfo.fileName, ios::in | ios::binary);

    // Find how long the file is, store that number, and return to the beginning of the file
    fileInfo.fileStream.seekg(0, fileInfo.fileStream.end);
    fileInfo.fileStreamLength = fileInfo.fileStream.tellg();
    fileInfo.fileStream.seekg(0, fileInfo.fileStream.beg);
}

map<int, int> getGlyphFrequencies(fileInfo &fileInfo) {
    map<int, int> glyphFrequencies;

    fileInfo.fileStream.seekg(0, ios::beg);

    // Putting values into a map and incrementing repeat offenders
    for (int i = 0; i < fileInfo.fileStreamLength; i++) {
        unsigned char c[1];
        fileInfo.fileStream.read((char *) c, 1);
        fileInfo.fileStream.seekg(0, 1);

        int asciiValue = getAsciiValue(c[0]);
        glyphFrequencies[asciiValue]++;
    }

    // Adding the eof character
    glyphFrequencies[256] = 1;

    return glyphFrequencies;
}


// TODO find better fileName
vector<huffTableEntry> createSortedVectorFromFile(fileInfo &fileInfo) {
    map<int, int> glyphFrequencies = getGlyphFrequencies(fileInfo);

    fileInfo.numberOfGlyphsInFile = glyphFrequencies.size();

    // Creates a vector that's as big as we need so that it can be sorted by value
    vector<huffTableEntry> huffTableVector(fileInfo.numberOfGlyphsInFile + (fileInfo.numberOfGlyphsInFile - 1));

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

vector<huffTableEntry> createHuffmanTable(fileInfo &fileInfo) {
    vector<huffTableEntry> huffTable = createSortedVectorFromFile(fileInfo);

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

    for (int i = 0; i < huffTable.size(); i++) {
        cout << i << ": " << huffTable[i].glyph << " " << huffTable[i].frequency << " "
             << huffTable[i].leftPointer
             << " " << huffTable[i].rightPointer << endl;
    }

    return huffTable;
}

map<char, string> generateByteCodeTable(vector<huffTableEntry> &huffTable) {
    map<char, string> byteCodes;
    string byteCode;

    generateByteCodes(huffTable, byteCodes, 0, byteCode);

    // Debug statement
    for (auto elem : byteCodes) {
        std::cout << elem.first << " " << std::hex << std::setfill('0') << std::setw(2) << std::uppercase << elem.second
                  << "\n";
    }
    return byteCodes;
}

string encode(fileInfo &fileInfo, map<char, string> &map) {
    fileInfo.fileStream;

    fileInfo.fileStream.seekg(0, ios::beg);

    string s = "";
    for (int i = 0; i < fileInfo.fileStreamLength; i++) {
        unsigned char c[1];
        fileInfo.fileStream.read((char *) c, 1);
        fileInfo.fileStream.seekg(0, 1);

        int asciiValue = getAsciiValue(c[0]);

        cout << "ASCII: " << asciiValue << endl << endl;
        string byteCode = map[asciiValue];
        cout << "BC: " << byteCode << endl << endl;
        s += byteCode;
    }

    cout << "S" << s << endl;
    return s;
}

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

void encodeMessageToBytes(string &message) {
    for (unsigned i = 0; i < message.length(); i += 8) {
        cout << message.substr(i, 8) << endl;

        // TODO handle message not being divisble by 8

        unsigned char c = encodeByte(message.substr(i, 8));
        cout << c << endl;
    }
}

void main() {
//    string fileToRead;
//    cout << "Enter the fileName of a file to be read: ";
//    getline(cin, fileToRead);
    string fileName = "Test.txt";

    fileInfo fileInfo;
    fileInfo.fileName = fileName;
    fileInfo.fileNameLength = fileName.length();

    loadFileContents(fileInfo);
    vector<huffTableEntry> huffTable = createHuffmanTable(fileInfo);
    map<char, string> encodingMap = generateByteCodeTable(huffTable);

    string message = encode(fileInfo, encodingMap);

    encodeMessageToBytes(message);

    // Output all necessary info to file

//    createAndOutputfileInfo(fileInfo, fileInfo.fileStreamLength, huffTable);
}