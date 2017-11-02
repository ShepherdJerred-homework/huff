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

struct hufFile {
    int fileNameLength;
    string fileName;
    int numberOfGlyphs;
    ifstream originalFile;
    double originalFileLength;
};

// Min heaping
// Marks current position as smallest, gets left and right children. If left child is not outside the heap
// compares its frequency to the smallest frequency. If it is smaller, set left child to the smallest. Then does the same
// with right child. If smallest is not equal to the original position fed in, switch the values for current position and the
// smallest value. Then re-check (which may be unnecessary...I can't remember why that's there)
void minHeap(vector<huffTableEntry> &hufFile, int position, int heapSize) {
    int smallest = position;
    int left = 2 * position + 1;
    int right = 2 * position + 2;

    if (left <= heapSize && hufFile[left].frequency < hufFile[smallest].frequency) {
        smallest = left;
    }

    if (right <= heapSize && hufFile[right].frequency < hufFile[smallest].frequency) {
        smallest = right;
    }

    if (smallest != position) {
        huffTableEntry temp = hufFile[position];
        hufFile[position] = hufFile[smallest];
        hufFile[smallest] = temp;
        minHeap(hufFile, smallest, heapSize);
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

void createAndOutputHufFile(hufFile hufFileInfo, double length) {
    // Strips away any extension from filename. If there isn't one, then just creates
    // a copy of the original filename.
    int pos = hufFileInfo.fileName.find_last_of(".");
    string fileName = hufFileInfo.fileName.substr(0, pos);

    string hufFile = fileName + ".huf";

    ofstream fout(hufFile, ios::out);
}

int getAsciiValue(unsigned char c) {
    return (int) c;
}

// TODO close file
// TODO map file into memory (https://stackoverflow.com/questions/15138353/how-to-read-a-binary-file-into-a-vector-of-unsigned-chars)
void loadFileContents(hufFile &hufFile) {
    hufFile.originalFile = ifstream(hufFile.fileName, ios::in | ios::binary);

    // Find how long the file is, store that number, and return to the beginning of the file
    hufFile.originalFile.seekg(0, hufFile.originalFile.end);
    hufFile.originalFileLength = hufFile.originalFile.tellg();
    hufFile.originalFile.seekg(0, hufFile.originalFile.beg);
}

map<int, int> getGlyphFrequencies(hufFile &hufFile) {
    map<int, int> glyphFrequencies;

    hufFile.originalFile.seekg(0, ios::beg);

    // Putting values into a map and incrementing repeat offenders
    for (int i = 0; i < hufFile.originalFileLength; i++) {
        unsigned char c[1];
        hufFile.originalFile.read((char *) c, 1);
        hufFile.originalFile.seekg(0, 1);

        int asciiValue = getAsciiValue(c[0]);
        glyphFrequencies[asciiValue]++;
    }

    // Adding the eof character
    glyphFrequencies[256] = 1;

    return glyphFrequencies;
}


// TODO find better fileName
vector<huffTableEntry> createSortedVectorFromFile(hufFile &hufFile) {
    map<int, int> glyphFrequencies = getGlyphFrequencies(hufFile);

    hufFile.numberOfGlyphs = glyphFrequencies.size();

    // Creates a vector that's as big as we need so that it can be sorted by value
    vector<huffTableEntry> huffTableVector(hufFile.numberOfGlyphs + (hufFile.numberOfGlyphs - 1));

    // Put map into vector
    int arrayLocation = 0;
    for (auto entry : glyphFrequencies) {
        huffTableVector[arrayLocation].glyph = entry.first;
        huffTableVector[arrayLocation].frequency = entry.second;
        arrayLocation++;
    }

    sort(huffTableVector.begin(), huffTableVector.begin() + hufFile.numberOfGlyphs, sortByFrequency);

    return huffTableVector;
}

vector<huffTableEntry> createHuffmanTable(hufFile &hufFile) {
    vector<huffTableEntry> huffTable = createSortedVectorFromFile(hufFile);

    // Creating the full huffman table
    int heapEnd = hufFile.numberOfGlyphs - 1;
    int firstFreeSlot = hufFile.numberOfGlyphs;
    int marked;

    // Stops 2 early because the last merge doesn't need to be this intense
    for (int i = 0; i < (hufFile.numberOfGlyphs - 2); i++) {
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

string encode(hufFile &hufFile, map<char, string> &map) {
    hufFile.originalFile;

    hufFile.originalFile.seekg(0, ios::beg);

    string s = "";
    for (int i = 0; i < hufFile.originalFileLength; i++) {
        unsigned char c[1];
        hufFile.originalFile.read((char *) c, 1);
        hufFile.originalFile.seekg(0, 1);

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

void compressAndWrite(const string &fileName) {

    hufFile hufFile;
    hufFile.fileName = fileName;
    hufFile.fileNameLength = fileName.length();

    loadFileContents(hufFile);
    vector<huffTableEntry> huffTable = createHuffmanTable(hufFile);
    map<char, string> encodingMap = generateByteCodeTable(huffTable);

    string message = encode(hufFile, encodingMap);

    encodeMessageToBytes(message);

    // Output all necessary info to file

//    createAndOutputHufFile(hufFile, hufFile.originalFileLength);
}

void main() {
//    string fileToRead;
//    cout << "Enter the fileName of a file to be read: ";
//    getline(cin, fileToRead);
    compressAndWrite("Test.txt");
}