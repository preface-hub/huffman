#include "huffman.h"

// 打印使用说明
void printUsage() {
	printf("\n霍夫曼文件压缩工具 - 用法:\n");
	printf("========================================\n");
	printf("压缩文件/目录:\n");
	printf("  myzip -c <压缩包名> <文件1> [文件2] ...\n");
	printf("  myzip -c <压缩包名> <目录名>\\\n");
	printf("  myzip -c <压缩包名> *.txt\n\n");
	printf("解压:\n");
	printf("  myzip -d <压缩包名>\n");
	printf("  myzip -d <压缩包名> -o <输出目录>\n\n");
	printf("列出压缩包内容:\n");
	printf("  myzip -l <压缩包名>\n\n");
	printf("示例:\n");
	printf("  myzip -c output.hzp file1.txt file2.txt\n");
	printf("  myzip -d output.hzp -o ./output/\n");
	printf("  myzip -l output.hzp\n");
	printf("========================================\n");
}

// 解析命令行
int parseCommandLine(int argc, char *argv[], char *mode, char *archiveName, 
	char *outputDir, char ***files, int *fileCount) {
		if (argc < 2) {
			return 0;
		}
		
		*mode = 0;
		*archiveName = NULL;
		*outputDir = NULL;
		*files = NULL;
		*fileCount = 0;
		
		for (int i = 1; i < argc; i++) {
			if (argv[i][0] == '-') {
				switch (argv[i][1]) {
				case 'c':
					*mode = 'c';
					if (i + 1 < argc) {
						*archiveName = argv[++i];
					}
					break;
				case 'd':
					*mode = 'd';
					if (i + 1 < argc) {
						*archiveName = argv[++i];
					}
					break;
				case 'l':
					*mode = 'l';
					if (i + 1 < argc) {
						*archiveName = argv[++i];
					}
					break;
				case 'o':
					if (i + 1 < argc) {
						*outputDir = argv[++i];
					}
					break;
				}
			} else if (argv[i][0] != '-') {
				if (*mode == 'c' && archiveName && strcmp(argv[i], *archiveName) != 0) {
					(*fileCount)++;
				}
			}
		}
		
		if (*mode == 'c' && *fileCount > 0) {
			*files = (char**)malloc(*fileCount * sizeof(char*));
			int idx = 0;
			for (int i = 1; i < argc; i++) {
				if (argv[i][0] == '-') {
					i++;  // 跳过参数值
					continue;
				}
				if (argv[i][0] != '-' && archiveName && strcmp(argv[i], archiveName) != 0) {
					(*files)[idx++] = argv[i];
				}
			}
		}
		
		return 1;
	}

// 目录遍历回调收集文件
typedef struct {
	char **files;
	int count;
	int capacity;
} FileCollector;

void addFileToList(const char *path, FileCollector *collector) {
	if (collector->count >= collector->capacity) {
		collector->capacity *= 2;
		collector->files = (char**)realloc(collector->files, collector->capacity * sizeof(char*));
	}
	collector->files[collector->count] = (char*)malloc(strlen(path) + 1);
	strcpy(collector->files[collector->count], path);
	collector->count++;
}

void collectFiles(const char *path, FileCollector *collector) {
	traverseDirectory(path, (void(*)(const char*))addFileToList);
}

// 遍历目录（跨平台）
void traverseDirectory(const char *path, void (*callback)(const char*)) {
#ifdef _WIN32
	WIN32_FIND_DATA findData;
	char searchPath[MAX_FILENAME * 2];
	snprintf(searchPath, sizeof(searchPath), "%s\\*", path);
	
	HANDLE hFind = FindFirstFile(searchPath, &findData);
	if (hFind == INVALID_HANDLE_VALUE) return;
	
	do {
		if (strcmp(findData.cFileName, ".") == 0 || strcmp(findData.cFileName, "..") == 0)
			continue;
		
		char fullPath[MAX_FILENAME * 2];
		snprintf(fullPath, sizeof(fullPath), "%s\\%s", path, findData.cFileName);
		
		if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			traverseDirectory(fullPath, callback);
		} else {
			callback(fullPath);
		}
	} while (FindNextFile(hFind, &findData));
	
	FindClose(hFind);
#else
	DIR *dir = opendir(path);
	if (!dir) return;
	
	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL) {
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
			continue;
		
		char fullPath[MAX_FILENAME * 2];
		snprintf(fullPath, sizeof(fullPath), "%s/%s", path, entry->d_name);
		
		struct stat st;
		if (stat(fullPath, &st) == 0) {
			if (S_ISDIR(st.st_mode)) {
				traverseDirectory(fullPath, callback);
			} else {
				callback(fullPath);
			}
		}
	}
	closedir(dir);
#endif
}

// 简单通配符匹配
int matchPattern(const char *filename, const char *pattern) {
	if (*pattern == '\0') return *filename == '\0';
	
	if (*pattern == '*') {
		do {
			if (matchPattern(filename, pattern + 1)) return 1;
		} while (*filename++);
		return 0;
	}
	
	if (*pattern == '?' || *pattern == *filename) {
		return matchPattern(filename + 1, pattern + 1);
	}
	return 0;
}

// 根据模式展开文件列表
void expandPatterns(char **patterns, int patternCount, FileCollector *collector) {
	for (int i = 0; i < patternCount; i++) {
		char *pattern = patterns[i];
		
		// 检查是否包含通配符
		if (strchr(pattern, '*') || strchr(pattern, '?')) {
			// 获取目录部分
			char dirPath[MAX_FILENAME];
			char filePattern[MAX_FILENAME];
			
			char *lastSep = strrchr(pattern, PATH_SEP);
			if (lastSep) {
				size_t dirLen = lastSep - pattern;
				strncpy(dirPath, pattern, dirLen);
				dirPath[dirLen] = '\0';
				strcpy(filePattern, lastSep + 1);
			} else {
				strcpy(dirPath, ".");
				strcpy(filePattern, pattern);
			}
			
			// 遍历目录匹配
			traverseDirectory(dirPath, (void(*)(const char*))addFileToList);
		} else {
			// 普通文件或目录
			struct stat st;
			if (stat(pattern, &st) == 0) {
				if (S_ISDIR(st.st_mode)) {
					traverseDirectory(pattern, collectFiles);
				} else {
					addFileToList(pattern, collector);
				}
			} else {
				printf("警告：找不到 %s\n", pattern);
			}
		}
	}
}

int main(int argc, char *argv[]) {
	char mode;
	char *archiveName;
	char *outputDir = NULL;
	char **files = NULL;
	int fileCount = 0;
	
	if (!parseCommandLine(argc, argv, &mode, &archiveName, &outputDir, &files, &fileCount)) {
		printUsage();
		return 1;
	}
	
	double startTime = getCurrentTime();
	
	switch (mode) {
	case 'c':
		if (fileCount == 0) {
			printf("错误：请指定要压缩的文件\n");
			printUsage();
			return 1;
		}
		
		// 展开通配符和目录
		FileCollector collector = {0};
		collector.capacity = 64;
		collector.files = (char**)malloc(collector.capacity * sizeof(char*));
		expandPatterns(files, fileCount, &collector);
		
		if (collector.count == 0) {
			printf("错误：没有找到要压缩的文件\n");
			free(collector.files);
			return 1;
		}
		
		printf("找到 %d 个文件\n", collector.count);
		compressArchive(archiveName, collector.files, collector.count);
		
		for (int i = 0; i < collector.count; i++) {
			free(collector.files[i]);
		}
		free(collector.files);
		break;
		
	case 'd':
		decompressArchive(archiveName, outputDir);
		break;
		
	case 'l':
		listArchive(archiveName);
		break;
		
	default:
		printUsage();
		free(files);
		return 1;
	}
	
	double endTime = getCurrentTime();
	printf("\n执行时间: %.3f 秒\n", endTime - startTime);
	
	if (files) free(files);
	return 0;
}
