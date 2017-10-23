#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <map>
#include <vector>
#include <algorithm>

using namespace std;


//Single entry in huffman table has a glyph, frequency, left pointer and right pointer
struct hufTableValues
{
	int glyph = -1;
	int frequency = 0;
	int leftPointer = -1;
	int rightPointer = -1;
};

//Probably unnecessary, and will likely need to be changed for sake of speed, but just had it here to keep things organized
struct hufFile
{
	int nameLength;
	string Name;
	int hufTableLength;
	map<int, int> fileValues;
	unsigned char* contents;
};


//Min heaping
//Marks current position as smallest, gets left and right children. If left child is not outside the heap
//compares its frequency to the smallest frequency. If it is smaller, set left child to the smallest. Then does the same
//with right child. If smallest is not equal to the original position fed in, switch the values for current position and the 
//smallest value. Then re-check (which may be unnecessary...I can't remember why that's there)
void minHeap(vector<hufTableValues> &hufFile, int position, int heapSize)
{
	int smallest = position;
	int left = 2 * position + 1;
	int right = 2 * position + 2;

	if (left <= heapSize && hufFile[left].frequency < hufFile[smallest].frequency)
	{
		smallest = left;
	}

	if (right <= heapSize && hufFile[right].frequency < hufFile[smallest].frequency)
	{
		smallest = right;
	}

	if (smallest != position)
	{
		hufTableValues temp = hufFile[position];
		hufFile[position] = hufFile[smallest];
		hufFile[smallest] = temp;
		minHeap(hufFile, smallest, heapSize);
	}
}

void generateByteCodes(vector<hufTableValues> &treeValues, map<char, string> &byteCodes, int position, string byteCode)
{
	if (treeValues[position].leftPointer != -1 || treeValues[position].rightPointer != -1)
	{
		if (treeValues[position].leftPointer != -1)
		{
			generateByteCodes(treeValues, byteCodes, treeValues[position].leftPointer, (byteCode + "0"));
		}
		if (treeValues[position].rightPointer != -1)
		{
			generateByteCodes(treeValues, byteCodes, treeValues[position].rightPointer, (byteCode + "1"));
		}
	}
	else
	{
		byteCodes[treeValues[position].glyph] = byteCode;
	}
}

// Sort hufTable by frequency
bool sortByFrequency(hufTableValues &lhs, hufTableValues &rhs)
{
	return lhs.frequency < rhs.frequency;
}

void createAndOutputHufFile(hufFile hufFileInfo, double length)
{
	//Strips away any extension from filename. If there isn't one, then just creates
	//a copy of the original filename.
	int pos = hufFileInfo.Name.find_last_of(".");
	string fileName = hufFileInfo.Name.substr(0, pos);

	string hufFile = fileName + ".huf";

	ofstream fout(hufFile, ios::out);

}

void performHuff(ifstream &fin, string fileToRead)
{
	hufFile hufFileInfo;
	hufFileInfo.Name = fileToRead;
	hufFileInfo.nameLength = fileToRead.length();
	//Find how long the file is, store that number, and return to the beginning of the file
	fin.seekg(0, fin.end);
	double length = fin.tellg();
	fin.seekg(0, fin.beg);

	//Create an unsigned char array to hold the contents of memory
	hufFileInfo.contents = new unsigned char[length];
	fin.read((char*)hufFileInfo.contents, length);

	//Putting values into a map and incrementing repeat offenders
	for (int i = 0; i < length; i++)
	{
		hufFileInfo.fileValues[(int)hufFileInfo.contents[i]]++;
	}

	//Adding the eof character
	hufFileInfo.fileValues[256] = 1;

	/*for (auto elem : hufFileInfo.fileValues)
	{
	std::cout << elem.first << " " << elem.second << "\n";
	}*/

	//Gets size to use later
	int fileValuesSize = hufFileInfo.fileValues.size();

	//Creates a vector thats as big as we need so that it can be sorted by value
	vector<hufTableValues> treeValues(fileValuesSize + (fileValuesSize - 1));

	//Adds map values to vector
	int arrayLocation = 0;
	for (auto elem : hufFileInfo.fileValues)
	{
		treeValues[arrayLocation].glyph = elem.first;
		treeValues[arrayLocation].frequency = elem.second;
		arrayLocation++;
	}

	//Sorts vector
	sort(treeValues.begin(), treeValues.begin() + fileValuesSize, sortByFrequency);


	//Creating the full huffman table
	int heapEnd = fileValuesSize - 1;
	int firstFreeSlot = fileValuesSize;
	int marked;
	//Stops 2 early because the last merge doesn't need to be this intense
	for (int i = 0; i < (fileValuesSize - 2); i++)
	{
		//Pieces of commented out code are for watching the table change and double checking it's right
		//I looked it over once already and it seems ok, I just don't wanna type all this junk out again
		//if something breaks
		/*hufTableValues tester;
		tester.glyph = 0;
		tester.frequency = 0;
		tester.leftPointer = 0;
		tester.rightPointer = 0;
		for (int j = 0; j < treeValues.size(); j++)
		{
		cout << treeValues[j].glyph << " " << treeValues[j].frequency << " " << treeValues[j].leftPointer << " " << treeValues[j].rightPointer << endl;
		}
		cout << endl << endl;*/
		marked = (treeValues[1].frequency < treeValues[2].frequency) ? 1 : 2;
		treeValues[firstFreeSlot] = treeValues[marked];
		treeValues[marked] = treeValues[heapEnd];
		/*treeValues[heapEnd] = tester;*/
		/*for (int j = 0; j < treeValues.size(); j++)
		{
		cout << treeValues[j].glyph << " " << treeValues[j].frequency << " " << treeValues[j].leftPointer << " " << treeValues[j].rightPointer << endl;
		}
		cout << endl << endl;*/
		for (int k = (heapEnd - 1) / 2; k >= 0; k--)
		{
			minHeap(treeValues, k, heapEnd - 1);
		}
		/*for (int j = 0; j < treeValues.size(); j++)
		{
		cout << treeValues[j].glyph << " " << treeValues[j].frequency << " " << treeValues[j].leftPointer << " " << treeValues[j].rightPointer << endl;
		}
		cout << endl << endl;*/
		treeValues[heapEnd] = treeValues[0];
		treeValues[0].glyph = -1;
		treeValues[0].frequency = treeValues[heapEnd].frequency + treeValues[firstFreeSlot].frequency;
		treeValues[0].leftPointer = heapEnd;
		treeValues[0].rightPointer = firstFreeSlot;
		/*for (int j = 0; j < treeValues.size(); j++)
		{
		cout << treeValues[j].glyph << " " << treeValues[j].frequency << " " << treeValues[j].leftPointer << " " << treeValues[j].rightPointer << endl;
		}
		cout << endl << endl;*/
		for (int l = (heapEnd - 1) / 2; l >= 0; l--)
		{
			minHeap(treeValues, l, heapEnd - 1);
		}
		/*for (int j = 0; j < treeValues.size(); j++)
		{
		cout << treeValues[j].glyph << " " << treeValues[j].frequency << " " << treeValues[j].leftPointer << " " << treeValues[j].rightPointer << endl;
		}
		cout << endl << endl;*/
		heapEnd--;
		firstFreeSlot++;
	}

	//Last merge
	treeValues[firstFreeSlot] = treeValues[0];
	treeValues[0].glyph = -1;
	treeValues[0].frequency = treeValues[1].frequency + treeValues[firstFreeSlot].frequency;
	treeValues[0].leftPointer = 1;
	treeValues[0].rightPointer = firstFreeSlot;

	for (int i = 0; i < treeValues.size(); i++)
	{
		cout << i << ": " << treeValues[i].glyph << " " << treeValues[i].frequency << " " << treeValues[i].leftPointer << " " << treeValues[i].rightPointer << endl;
	}

	//To do
	//-Generate bit values for leafs based on treeValues table

	map<char, string> byteCodes;
	string byteCode;

	generateByteCodes(treeValues, byteCodes, 0, byteCode);

	for (auto elem : byteCodes)
	{
	std::cout << elem.first << " " << elem.second << "\n";
	}
	//-Encode the message
	//-Output all necessary info to file

	createAndOutputHufFile(hufFileInfo, length);
}

void main()
{

	string fileToRead = "File";
	cout << "Enter the name of a file to be read: ";
	getline(cin, fileToRead);

	ifstream fin(fileToRead, ios::in | ios::binary);
	if (fin)
	{
		performHuff(fin, fileToRead);
		fin.close();
	}
	else
	{
		cout << endl << "There was an error opening the file" << endl << endl;
	}
}