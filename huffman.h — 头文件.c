#ifndef HUFFMAN_H
#define HUFFMAN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#define PATH_SEP '\\'
#define MKDIR(path) _mkdir(path)
#else
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#define PATH_SEP '/'
#define MKDIR(path) mkdir(path, 0755)
#endif

#include <time.h>

// 常量定义
#define MAGIC_NUMBER 0x48454631  // "HEF1"
#define MAX_FILENAME 256
#define BUFFER_SIZE 65536        // 64KB缓冲区
#define BLOCK_SIZE 4096

// Huffman树节点
typedef struct HuffmanNode {
	unsigned char data;
	unsigned int freq;
	struct HuffmanNode *left;
	struct HuffmanNode *right;
} HuffmanNode;

// 最小堆
typedef struct {
	HuffmanNode **array;
	int size;
	int capacity;
} MinHeap;

// 编码表
typedef struct {
	char *code[256];
	int len[256];
} CodeTable;

// 文件头（单文件）
typedef struct {
	unsigned int magic;
	char filename[MAX_FILENAME];
	unsigned long long orig_size;
	unsigned short char_count;
} FileHeader;

// 多文件归档头
typedef struct {
	char filename[MAX_FILENAME];
	unsigned long long orig_size;
	unsigned long long comp_size;
	time_t mtime;
} ArchiveEntry;

// 位写入器
typedef struct {
	FILE *fp;
	unsigned char buffer;
	int bit_count;
} BitWriter;

// 位读取器
typedef struct {
	FILE *fp;
	unsigned char buffer;
	int bit_count;
} BitReader;

// 函数声明
// Huffman核心
MinHeap* createMinHeap(int capacity);
HuffmanNode* createNode(unsigned char data, unsigned int freq);
void insertMinHeap(MinHeap *heap, HuffmanNode *node);
HuffmanNode* extractMin(MinHeap *heap);
HuffmanNode* buildHuffmanTree(unsigned int freq[256]);
void generateCodes(HuffmanNode *root, char *code, int depth, CodeTable *table);
void freeHuffmanTree(HuffmanNode *root);

// 位操作
void initBitWriter(BitWriter *bw, FILE *fp);
void writeBit(BitWriter *bw, int bit);
void flushBits(BitWriter *bw);
void initBitReader(BitReader *br, FILE *fp);
int readBit(BitReader *br);
void closeBitReader(BitReader *br);

// 压缩解压核心
int compressSingleFile(const char *inputPath, const char *outputPath);
int decompressSingleFile(const char *inputPath, const char *outputPath);

// 多文件和目录
int compressArchive(const char *archiveName, char **files, int fileCount);
int decompressArchive(const char *archiveName, const char *outputDir);
int listArchive(const char *archiveName);

// 工具函数
unsigned long long getFileSize(const char *path);
int fileExists(const char *path);
void traverseDirectory(const char *path, void (*callback)(const char*));
int matchPattern(const char *filename, const char *pattern);
double getCurrentTime();

// CLI
void printUsage();
int parseCommandLine(int argc, char *argv[], char *mode, char *archiveName, 
	char *outputDir, char ***files, int *fileCount);

#endif
