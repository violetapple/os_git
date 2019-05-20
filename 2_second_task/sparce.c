#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

/*
справка, чтобы не переключаться на браузер, man'ы, etc.

============================================================
int open(file, mode, flags_if_created)
flags:
S_IRUSR Set read rights for the owner to true.
S_IWUSR Set write rights for the owner to true.
S_IXUSR Set execution rights for the owner to true.
S_IRGRP Set read rights for the group to true.
S_IWGRP Set write rights for the group to true.
S_IXGRP Set execution rights for the group to true.
S_IROTH Set read rights for other users to true.
S_IWOTH Set write rights for other users to true.
S_IXOTH Set execution rights for other users to true.

============================================================
long lseek(int fd, long offset, int origin)

Origin  Результат обращения к lseek()
0       Смещение отсчитывается от начала файла
1       Смещение отсчитывается от текущей позиции
2       Смещение отсчитывается от конца файла

SEEK_SET 0 
SEEK_CUR 1 
SEEK_END 2 

============================================================
*/

const int BUFFER_LENGTH = 100;
const int INPUT_STREAM_DESC = STDIN_FILENO;

//Обертки для вывода сообщений об ошибках

int open_file_with_info(char* file_name) {
    //какие флаги 3-им аргументом выставлять по канону?
    int fd = open(file_name, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
    if (fd < 0) {
        printf("Error with opening file\n");
    }
    return fd;
}

int close_file_with_info(int fd) {
    int close_result = close(fd);
    if(close_result < 0) {
        printf("Error with closing file\n");
    }
    return close_result;
}

int read_with_info(int fd, void *read_buffer) {
    int bytes_read = read(INPUT_STREAM_DESC, read_buffer, BUFFER_LENGTH);
    if(bytes_read < 0) {
        printf("Error with reading\n");
    }
    return bytes_read;
}

int write_with_info(int fd, void *write_buffer, int write_len) {
    int write_code = write(fd, write_buffer, write_len);
    if(write_code < 0) {
        printf("Error with writing to file\n");
    }
    return write_code;
}

int lseek_with_info(int fd, long zero_chain_length) {
    int lseek_code = lseek(fd, zero_chain_length, SEEK_CUR);
    if(lseek_code < 0) {
        printf("Error with lseek\n");
    }
    return lseek_code;
}

int make_sparse_file(char *sparse_file) {

    int fd = open_file_with_info(sparse_file);
    if (fd < 0) {
        return fd;
    }

    char write_buffer[BUFFER_LENGTH];
    char read_buffer[BUFFER_LENGTH];
    int zero_chain_length = 0;
    int output_data_length = 0;
    int bytes_has_been_read;
    int input_data_index;
    // результат операций храним тут
    int operation_code;

    while (1) {
        //читаем байты из STDIN
        bytes_has_been_read = read_with_info(INPUT_STREAM_DESC, read_buffer);
        if(bytes_has_been_read < 0) {
            return bytes_has_been_read;
        }
        input_data_index = 0;
        if (bytes_has_been_read) {
            while (input_data_index < bytes_has_been_read) {

                while (read_buffer[input_data_index] != 0 && input_data_index < bytes_has_been_read) {
                    write_buffer[output_data_length] = read_buffer[input_data_index];
                    output_data_length += 1;
                    input_data_index += 1;
                }

                // сбрасываем кусками инфу в файл
                if (output_data_length > 0) {
                    operation_code = write_with_info(fd, write_buffer, output_data_length);
                    if (operation_code < 0) {
                        return operation_code;
                    }
                    output_data_length = 0;
                }

                //отслеживаем цепочку нулей
                while (1) {
                    if(read_buffer[input_data_index] == 0 && input_data_index < bytes_has_been_read) {
                        zero_chain_length += 1;
                        input_data_index += 1;
                    } else {
                        break;
                    }
                }
                
                if (zero_chain_length > 0) {
                    operation_code = lseek(fd, zero_chain_length, SEEK_CUR);
                    if(operation_code < 0) {
                        return operation_code;
                    }
                    zero_chain_length = 0;
                }
            }
        } else {
            break;
        }
    }
    return close(fd);
}

int main(int argc, char *argv[]) {
    if(argc < 2) {
        printf("[-] ERROR: please choose output file\n");
        return -1;
    }
    if (make_sparse_file(argv[1]) < 0) {
        printf("[-] ERROR: The errors are shown above ^\n");
        return -1;
    } else {
        printf("[+] SUCCESS\n");
    }
    return 0;
}

