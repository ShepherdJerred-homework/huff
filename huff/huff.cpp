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

void
generateByteCodes(vector<HuffTableEntry> &treeValues, map<char, string> &byteCodes, int position, string byteCode) {
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

int getAsciiValue(unsigned char c) {
    return (int) c;
}

// TODO close file
// TODO map file into memory (https://stackoverflow.com/questions/15138353/how-to-read-a-binary-file-into-a-vector-of-unsigned-chars)
void loadFileContents(FileInfo &fileInfo) {
    fileInfo.fileStream = ifstream(fileInfo.fileName, ios::in | ios::binary);

    // Find how long the file is, store that number, and return to the beginning of the file
    fileInfo.fileStream.seekg(0, fileInfo.fileStream.end);
    fileInfo.fileStreamLength = fileInfo.fileStream.tellg();
    fileInfo.fileStream.seekg(0, fileInfo.fileStream.beg);
}

map<int, int> getGlyphFrequencies(FileInfo &fileInfo) {
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
    glyphFrequencies[257] = 1;

    return glyphFrequencies;
}


// TODO find better fileName
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

//    for (int i = 0; i < huffTable.size(); i++) {
//        cout << i << ": " << huffTable[i].glyph << " " << huffTable[i].frequency << " "
//             << huffTable[i].leftPointer
//             << " " << huffTable[i].rightPointer << endl;
//    }

    return huffTable;
}

map<char, string> generateByteCodeTable(vector<HuffTableEntry> &huffTable) {
    map<char, string> byteCodes;
    string byteCode;

    generateByteCodes(huffTable, byteCodes, 0, byteCode);

//     Debug statement
//    for (auto elem : byteCodes) {
//        std::cout << elem.first << " " << std::hex << std::setfill('0') << std::setw(2) << std::uppercase << elem.second
//                  << "\n";
//    }
    return byteCodes;
}

/**
 * Reads through every byte in a file, and encodes it using a map
 * @param fileInfo The fileInfo object to use; contains the bytes to be encoded
 * @param map The map to encode with
 * @return An encoded string of 0's and 1's
 */
string encodeMessageToStringOfBits(FileInfo &fileInfo, map<char, string> &map) {
    fileInfo.fileStream.seekg(0, ios::beg);

    string s = "";
    for (int i = 0; i < fileInfo.fileStreamLength; i++) {
        unsigned char c[1];
        fileInfo.fileStream.read((char *) c, 1);
        fileInfo.fileStream.seekg(0, 1);

        int asciiValue = getAsciiValue(c[0]);

        string byteCode = map[asciiValue];
//        std::reverse(byteCode.begin(), byteCode.end());

//        cout << "ASCII: " << (char) asciiValue << "|" << asciiValue << endl;
//        cout << "Byte Code: " << byteCode << endl << endl;

        s += byteCode;
    }

    // Add the eof character
    string byteCode = map[256];
//    std::reverse(byteCode.begin(), byteCode.end());
    s += byteCode;

//    cout << "Encoded message:  " << s << endl;
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
        if (newMessage == "") {
            cout << newMessage;
            continue;
        }

        if (newMessage.length() < 8 && newMessage.length() > 0) {
            int numberNeeded = 8 - newMessage.length();
            for (int x = 0; x < numberNeeded; x++) {
                newMessage += "0";
            }
            break;
        }

//        cout << newMessage << endl;

        unsigned char c = encodeByte(newMessage);

        bytes += c;
//        cout << "Byte: " << c << endl;
//        cout << std::hex << "Hex Byte: " << (int) c << endl;
    }
//    cout << "Bytes: " << bytes << endl;
    return bytes;
}

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

//    string fileToRead;
//    cout << "Enter the fileName of a file to be read: ";
//    getline(cin, fileToRead);

    string fileName = "KiTTY.exe";

    FileInfo fileInfo;
    fileInfo.fileName = fileName;
    fileInfo.fileNameLength = fileName.length();

    loadFileContents(fileInfo);
    vector<HuffTableEntry> huffTable = createHuffmanTable(fileInfo);
    map<char, string> encodingMap = generateByteCodeTable(huffTable);

    string message = encodeMessageToStringOfBits(fileInfo, encodingMap);

    string bytes = encodeMessageToStringOfBytes(message);

    createAndOutputFileInfo(fileInfo, huffTable, bytes);

    fileInfo.fileStream.close();

}