#include "huffman.h"

// 创建最小堆
MinHeap* createMinHeap(int capacity) {
	MinHeap *heap = (MinHeap*)malloc(sizeof(MinHeap));
	heap->array = (HuffmanNode**)malloc(capacity * sizeof(HuffmanNode*));
	heap->size = 0;
	heap->capacity = capacity;
	return heap;
}

// 创建节点
HuffmanNode* createNode(unsigned char data, unsigned int freq) {
	HuffmanNode *node = (HuffmanNode*)malloc(sizeof(HuffmanNode));
	node->data = data;
	node->freq = freq;
	node->left = node->right = NULL;
	return node;
}

// 交换节点
void swapNode(HuffmanNode **a, HuffmanNode **b) {
	HuffmanNode *temp = *a;
	*a = *b;
	*b = temp;
}

// 最小堆调整
void minHeapify(MinHeap *heap, int idx) {
	int smallest = idx;
	int left = 2 * idx + 1;
	int right = 2 * idx + 2;
	
	if (left < heap->size && heap->array[left]->freq < heap->array[smallest]->freq)
		smallest = left;
	if (right < heap->size && heap->array[right]->freq < heap->array[smallest]->freq)
		smallest = right;
	
	if (smallest != idx) {
		swapNode(&heap->array[idx], &heap->array[smallest]);
		minHeapify(heap, smallest);
	}
}

// 插入节点
void insertMinHeap(MinHeap *heap, HuffmanNode *node) {
	heap->size++;
	int i = heap->size - 1;
	heap->array[i] = node;
	
	while (i > 0 && heap->array[(i - 1) / 2]->freq > heap->array[i]->freq) {
		swapNode(&heap->array[i], &heap->array[(i - 1) / 2]);
		i = (i - 1) / 2;
	}
}

// 提取最小值
HuffmanNode* extractMin(MinHeap *heap) {
	if (heap->size <= 0) return NULL;
	if (heap->size == 1) {
		heap->size--;
		return heap->array[0];
	}
	
	HuffmanNode *root = heap->array[0];
	heap->array[0] = heap->array[heap->size - 1];
	heap->size--;
	minHeapify(heap, 0);
	return root;
}

// 构建Huffman树
HuffmanNode* buildHuffmanTree(unsigned int freq[256]) {
	MinHeap *heap = createMinHeap(256);
	
	for (int i = 0; i < 256; i++) {
		if (freq[i] > 0) {
			insertMinHeap(heap, createNode((unsigned char)i, freq[i]));
		}
	}
	
	while (heap->size > 1) {
		HuffmanNode *left = extractMin(heap);
		HuffmanNode *right = extractMin(heap);
		HuffmanNode *parent = createNode(0, left->freq + right->freq);
		parent->left = left;
		parent->right = right;
		insertMinHeap(heap, parent);
	}
	
	HuffmanNode *root = extractMin(heap);
	free(heap->array);
	free(heap);
	return root;
}

// 生成编码表
void generateCodes(HuffmanNode *root, char *code, int depth, CodeTable *table) {
	if (!root) return;
	
	if (!root->left && !root->right) {
		code[depth] = '\0';
		table->code[root->data] = (char*)malloc(depth + 1);
		strcpy(table->code[root->data], code);
		table->len[root->data] = depth;
		return;
	}
	
	if (root->left) {
		code[depth] = '0';
		generateCodes(root->left, code, depth + 1, table);
	}
	if (root->right) {
		code[depth] = '1';
		generateCodes(root->right, code, depth + 1, table);
	}
}

// 释放Huffman树
void freeHuffmanTree(HuffmanNode *root) {
	if (!root) return;
	freeHuffmanTree(root->left);
	freeHuffmanTree(root->right);
	free(root);
}

// 初始化位写入器
void initBitWriter(BitWriter *bw, FILE *fp) {
	bw->fp = fp;
	bw->buffer = 0;
	bw->bit_count = 0;
}

// 写入一位
void writeBit(BitWriter *bw, int bit) {
	bw->buffer = (bw->buffer << 1) | bit;
	bw->bit_count++;
	
	if (bw->bit_count == 8) {
		fwrite(&bw->buffer, 1, 1, bw->fp);
		bw->bit_count = 0;
		bw->buffer = 0;
	}
}

// 刷新剩余位
void flushBits(BitWriter *bw) {
	if (bw->bit_count > 0) {
		bw->buffer <<= (8 - bw->bit_count);
		fwrite(&bw->buffer, 1, 1, bw->fp);
	}
}

// 初始化位读取器
void initBitReader(BitReader *br, FILE *fp) {
	br->fp = fp;
	br->buffer = 0;
	br->bit_count = 0;
}

// 读取一位
int readBit(BitReader *br) {
	if (br->bit_count == 0) {
		if (fread(&br->buffer, 1, 1, br->fp) != 1)
			return -1;
		br->bit_count = 8;
	}
	br->bit_count--;
	return (br->buffer >> br->bit_count) & 1;
}

// 关闭位读取器
void closeBitReader(BitReader *br) {
	// 无需特殊操作
}

// 获取文件大小
unsigned long long getFileSize(const char *path) {
	FILE *fp = fopen(path, "rb");
	if (!fp) return 0;
	fseek(fp, 0, SEEK_END);
	unsigned long long size = ftell(fp);
	fclose(fp);
	return size;
}

// 检查文件是否存在
int fileExists(const char *path) {
	FILE *fp = fopen(path, "rb");
	if (fp) {
		fclose(fp);
		return 1;
	}
	return 0;
}

// 获取当前时间（秒）
double getCurrentTime() {
#ifdef _WIN32
	LARGE_INTEGER freq, count;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&count);
	return (double)count.QuadPart / freq.QuadPart;
#else
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_sec + ts.tv_nsec / 1000000000.0;
#endif
}

// 保存Huffman树到文件
void saveHuffmanTree(BitWriter *bw, HuffmanNode *root) {
	if (!root) return;
	
	if (!root->left && !root->right) {
		// 叶子节点：写入1 + 字符
		writeBit(bw, 1);
		for (int i = 7; i >= 0; i--) {
			writeBit(bw, (root->data >> i) & 1);
		}
	} else {
		// 内部节点：写入0
		writeBit(bw, 0);
		saveHuffmanTree(bw, root->left);
		saveHuffmanTree(bw, root->right);
	}
}

// 从文件加载Huffman树
HuffmanNode* loadHuffmanTree(BitReader *br) {
	int bit = readBit(br);
	if (bit == -1) return NULL;
	
	if (bit == 1) {
		// 叶子节点
		unsigned char data = 0;
		for (int i = 7; i >= 0; i--) {
			int b = readBit(br);
			if (b == 1) data |= (1 << i);
		}
		return createNode(data, 0);
	} else {
		// 内部节点
		HuffmanNode *node = createNode(0, 0);
		node->left = loadHuffmanTree(br);
		node->right = loadHuffmanTree(br);
		return node;
	}
}

// 压缩单个文件
int compressSingleFile(const char *inputPath, const char *outputPath) {
	FILE *in = fopen(inputPath, "rb");
	if (!in) {
		printf("错误：无法打开输入文件 %s\n", inputPath);
		return 0;
	}
	
	FILE *out = fopen(outputPath, "wb");
	if (!out) {
		printf("错误：无法创建输出文件 %s\n", outputPath);
		fclose(in);
		return 0;
	}
	
	// 统计频率
	unsigned int freq[256] = {0};
	unsigned char buffer[BUFFER_SIZE];
	size_t bytes;
	
	while ((bytes = fread(buffer, 1, BUFFER_SIZE, in)) > 0) {
		for (size_t i = 0; i < bytes; i++) {
			freq[buffer[i]]++;
		}
	}
	
	// 构建Huffman树
	HuffmanNode *root = buildHuffmanTree(freq);
	CodeTable table = {0};
	char code[256];
	generateCodes(root, code, 0, &table);
	
	// 写入文件头
	FileHeader header;
	header.magic = MAGIC_NUMBER;
	memset(header.filename, 0, MAX_FILENAME);
	const char *basename = strrchr(inputPath, PATH_SEP);
	if (basename) basename++;
	else basename = inputPath;
	strncpy(header.filename, basename, MAX_FILENAME - 1);
	header.orig_size = getFileSize(inputPath);
	
	// 统计有效字符数
	header.char_count = 0;
	for (int i = 0; i < 256; i++) {
		if (freq[i] > 0) header.char_count++;
	}
	
	fwrite(&header, sizeof(FileHeader), 1, out);
	
	// 写入频率表（作为树的备份）
	fwrite(freq, sizeof(unsigned int), 256, out);
	
	// 重新读取文件并压缩
	rewind(in);
	BitWriter bw;
	initBitWriter(&bw, out);
	
	// 写入Huffman树
	saveHuffmanTree(&bw, root);
	
	// 写入数据
	while ((bytes = fread(buffer, 1, BUFFER_SIZE, in)) > 0) {
		for (size_t i = 0; i < bytes; i++) {
			char *code = table.code[buffer[i]];
			if (code) {
				for (int j = 0; j < table.len[buffer[i]]; j++) {
					writeBit(&bw, code[j] - '0');
				}
			}
		}
	}
	
	flushBits(&bw);
	
	// 清理
	for (int i = 0; i < 256; i++) {
		if (table.code[i]) free(table.code[i]);
	}
	freeHuffmanTree(root);
	fclose(in);
	fclose(out);
	
	return 1;
}

// 解压单个文件
int decompressSingleFile(const char *inputPath, const char *outputPath) {
	FILE *in = fopen(inputPath, "rb");
	if (!in) {
		printf("错误：无法打开压缩文件 %s\n", inputPath);
		return 0;
	}
	
	// 读取文件头
	FileHeader header;
	if (fread(&header, sizeof(FileHeader), 1, in) != 1) {
		printf("错误：无效的压缩文件格式\n");
		fclose(in);
		return 0;
	}
	
	if (header.magic != MAGIC_NUMBER) {
		printf("错误：文件不是本程序压缩的格式\n");
		fclose(in);
		return 0;
	}
	
	// 读取频率表
	unsigned int freq[256];
	fread(freq, sizeof(unsigned int), 256, in);
	
	// 重建Huffman树
	HuffmanNode *root = buildHuffmanTree(freq);
	
	// 解压数据
	FILE *out = fopen(outputPath, "wb");
	if (!out) {
		printf("错误：无法创建输出文件 %s\n", outputPath);
		freeHuffmanTree(root);
		fclose(in);
		return 0;
	}
	
	BitReader br;
	initBitReader(&br, in);
	
	unsigned long long written = 0;
	unsigned char outBuffer[BUFFER_SIZE];
	int outPos = 0;
	
	while (written < header.orig_size) {
		HuffmanNode *curr = root;
		while (curr->left || curr->right) {
			int bit = readBit(&br);
			if (bit == -1) break;
			if (bit == 0) curr = curr->left;
			else curr = curr->right;
		}
		outBuffer[outPos++] = curr->data;
		written++;
		
		if (outPos >= BUFFER_SIZE) {
			fwrite(outBuffer, 1, outPos, out);
			outPos = 0;
		}
	}
	
	if (outPos > 0) {
		fwrite(outBuffer, 1, outPos, out);
	}
	
	freeHuffmanTree(root);
	fclose(in);
	fclose(out);
	
	return 1;
}

// 压缩多文件归档
int compressArchive(const char *archiveName, char **files, int fileCount) {
	FILE *archive = fopen(archiveName, "wb");
	if (!archive) {
		printf("错误：无法创建归档文件 %s\n", archiveName);
		return 0;
	}
	
	// 写入归档魔数
	unsigned int magic = MAGIC_NUMBER;
	fwrite(&magic, sizeof(unsigned int), 1, archive);
	
	// 写入文件数量
	fwrite(&fileCount, sizeof(int), 1, archive);
	
	for (int i = 0; i < fileCount; i++) {
		printf("压缩: %s\n", files[i]);
		
		char tempCompressed[MAX_FILENAME];
		snprintf(tempCompressed, sizeof(tempCompressed), "%s.tmp", files[i]);
		
		if (!compressSingleFile(files[i], tempCompressed)) {
			printf("压缩失败: %s\n", files[i]);
			fclose(archive);
			return 0;
		}
		
		// 获取压缩后大小
		unsigned long long compSize = getFileSize(tempCompressed);
		unsigned long long origSize = getFileSize(files[i]);
		
		// 获取文件修改时间
		time_t mtime = time(NULL);
		
		// 写入条目头
		ArchiveEntry entry;
		memset(entry.filename, 0, MAX_FILENAME);
		const char *basename = strrchr(files[i], PATH_SEP);
		if (basename) basename++;
		else basename = files[i];
		strncpy(entry.filename, basename, MAX_FILENAME - 1);
		entry.orig_size = origSize;
		entry.comp_size = compSize;
		entry.mtime = mtime;
		
		fwrite(&entry, sizeof(ArchiveEntry), 1, archive);
		
		// 写入压缩数据
		FILE *tmp = fopen(tempCompressed, "rb");
		if (!tmp) {
			printf("错误：无法读取临时文件\n");
			fclose(archive);
			return 0;
		}
		
		unsigned char buffer[BUFFER_SIZE];
		size_t bytes;
		while ((bytes = fread(buffer, 1, BUFFER_SIZE, tmp)) > 0) {
			fwrite(buffer, 1, bytes, archive);
		}
		
		fclose(tmp);
		remove(tempCompressed);  // 删除临时文件
	}
	
	fclose(archive);
	printf("压缩完成: %s\n", archiveName);
	return 1;
}

// 解压归档
int decompressArchive(const char *archiveName, const char *outputDir) {
	FILE *archive = fopen(archiveName, "rb");
	if (!archive) {
		printf("错误：无法打开归档文件 %s\n", archiveName);
		return 0;
	}
	
	// 读取魔数
	unsigned int magic;
	fread(&magic, sizeof(unsigned int), 1, archive);
	if (magic != MAGIC_NUMBER) {
		printf("错误：无效的归档格式\n");
		fclose(archive);
		return 0;
	}
	
	// 读取文件数量
	int fileCount;
	fread(&fileCount, sizeof(int), 1, archive);
	
	// 创建输出目录
	if (outputDir) {
#ifdef _WIN32
		MKDIR(outputDir);
#else
		mkdir(outputDir, 0755);
#endif
	}
	
	for (int i = 0; i < fileCount; i++) {
		ArchiveEntry entry;
		fread(&entry, sizeof(ArchiveEntry), 1, archive);
		
		// 构建输出路径
		char outputPath[MAX_FILENAME * 2];
		if (outputDir) {
			snprintf(outputPath, sizeof(outputPath), "%s%c%s", outputDir, PATH_SEP, entry.filename);
		} else {
			strcpy(outputPath, entry.filename);
		}
		
		printf("解压: %s -> %s\n", entry.filename, outputPath);
		
		// 创建临时压缩文件
		char tempCompressed[MAX_FILENAME];
		snprintf(tempCompressed, sizeof(tempCompressed), "%s.tmp", entry.filename);
		
		FILE *tmp = fopen(tempCompressed, "wb");
		if (!tmp) {
			printf("错误：无法创建临时文件\n");
			fclose(archive);
			return 0;
		}
		
		// 读取压缩数据
		unsigned char buffer[BUFFER_SIZE];
		unsigned long long remaining = entry.comp_size;
		while (remaining > 0) {
			size_t toRead = (remaining < BUFFER_SIZE) ? remaining : BUFFER_SIZE;
			size_t bytes = fread(buffer, 1, toRead, archive);
			fwrite(buffer, 1, bytes, tmp);
			remaining -= bytes;
		}
		
		fclose(tmp);
		
		// 解压临时文件
		if (!decompressSingleFile(tempCompressed, outputPath)) {
			printf("解压失败: %s\n", entry.filename);
			remove(tempCompressed);
			fclose(archive);
			return 0;
		}
		
		remove(tempCompressed);
	}
	
	fclose(archive);
	printf("解压完成\n");
	return 1;
}

// 列出归档内容
int listArchive(const char *archiveName) {
	FILE *archive = fopen(archiveName, "rb");
	if (!archive) {
		printf("错误：无法打开归档文件 %s\n", archiveName);
		return 0;
	}
	
	unsigned int magic;
	fread(&magic, sizeof(unsigned int), 1, archive);
	if (magic != MAGIC_NUMBER) {
		printf("错误：无效的归档格式\n");
		fclose(archive);
		return 0;
	}
	
	int fileCount;
	fread(&fileCount, sizeof(int), 1, archive);
	
	printf("\n归档文件: %s\n", archiveName);
	printf("包含 %d 个文件:\n\n", fileCount);
	printf("%-30s %12s %12s\n", "文件名", "原始大小", "压缩后大小");
	printf("------------------------------------------------\n");
	
	unsigned long long totalOrig = 0, totalComp = 0;
	
	for (int i = 0; i < fileCount; i++) {
		ArchiveEntry entry;
		fread(&entry, sizeof(ArchiveEntry), 1, archive);
		printf("%-30s %12llu %12llu\n", entry.filename, entry.orig_size, entry.comp_size);
		totalOrig += entry.orig_size;
		totalComp += entry.comp_size;
		
		// 跳过数据
		fseek(archive, entry.comp_size, SEEK_CUR);
	}
	
	printf("------------------------------------------------\n");
	printf("%-30s %12llu %12llu\n", "总计", totalOrig, totalComp);
	if (totalOrig > 0) {
		printf("\n压缩比: %.2f%%\n", (1 - (double)totalComp / totalOrig) * 100);
	}
	
	fclose(archive);
	return 1;
}
