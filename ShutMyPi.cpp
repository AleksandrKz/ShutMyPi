// ShutMyPi.cpp: определяет точку входа для приложения.
//

#include "ShutMyPi.h"

//using namespace std;
using namespace std::filesystem;
using namespace std::literals;

std::string sha256(const std::string str);

uint16_t port = 45111;

std::string pass_hash;

bool pass_is_set = false;

const std::string http_ok = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
const std::string http_400 = "HTTP/1.1 400 OK\r\nContent-Type: text/plain\r\n\r\n No found";
const std::string index_page_def = "<!DOCTYPE html>\r\n<html lang=\"ru\">\r\n\t<head>\r\n\t\t<meta charset=\"utf-8\">\r\n\t\t<meta name=\"viewport\" content=\"width=device-width, user-scalable=yes\">\r\n\t\t<title>ShutMyPi</title>\r\n\t\t<style type=\"text/css\">\r\n\t\t\th3 {\r\n\t\t\t\ttext-align: center;\r\n\t\t\t\tmargin-bottom: 2em;\r\n\t\t\t}\r\n\t\t\t.btn-reb {\r\n\t\t\t\ttext-decoration: none;\r\n\t\t\t\tbackground: lavender;\r\n\t\t\t\tcolor: black;\r\n\t\t\t\ttext-align: center; \r\n\t\t\t\tborder: 0px solid black;\r\n\t\t\t\tborder-radius: 10px;\r\n\t\t\t\tmargin: 10px auto; \r\n\t\t\t\tpadding: 25px 1em;\r\n\t\t\t}\r\n\t\t\t.btn-shut {\r\n\t\t\t\ttext-decoration: none;\r\n\t\t\t\tbackground: pink;\r\n\t\t\t\tcolor: black;\r\n\t\t\t\ttext-align: center; \r\n\t\t\t\tborder: 0px solid black;\r\n\t\t\t\tborder-radius: 10px;\r\n\t\t\t\tmargin: 10px auto; \r\n\t\t\t\tpadding: 25px 2em;\r\n\t\t\t}\r\n\t\t\t.nav {\r\n\t\t\t\twidth: 50%;\r\n\t\t\t\ttext-align: center;\r\n\t\t\t\tmargin-bottom: 3em;\r\n\t\t\t\tfloat: left;\r\n\t\t\t}\r\n\t\t\t.command {\r\n\t\t\t\twidth: 100%;\r\n\t\t\t}\r\n\t\t</style>\r\n\t</head>\r\n\t<body>\r\n\t\t<div>\t\r\n\t\t\t<h3>Команды:</h3>\r\n\t\t\t<div class=\"command\">\r\n\t\t\t\t<div class=\"nav\">\r\n\t\t\t\t\t<a href=\"/shutdown\" class=\"btn-shut\">Выключить</a>\r\n\t\t\t\t</div>\r\n\t\t\t\t<div class=\"nav\">\r\n\t\t\t\t\t<a href=\"/reboot\" class=\"btn-reb\">Перезагрузить</a>\r\n\t\t\t\t</div>\r\n\t\t\t</div>\r\n\t\t</div>\r\n\t</body>\r\n</html>"s;
const std::string shutdown_str = "<h1 style=\"text-align: center; \">Shutdown...</h1>";
const std::string reboot_str = "<h1 style=\"text-align: center; \">Reboot...</h1>";
std::string index_page;

const path index_file_dir{ std::filesystem::current_path() / "index.html" };
const path pass_file_dir{ std::filesystem::current_path() / "pass.txt" };

std::regex regexPathPass("^GET\\s+(/|/shutdown|/reboot)(?:\\?pass=(\\S+))?\\s+HTTP\\/\\d{1}\\.\\d{1}"s, std::regex_constants::icase);

int main(int argc, char* argv[])
{
	if (argc > 2 && argc < 4)
	{
		for (int i = 1; i < argc; ++i)
		{
			std::string argument(argv[i]);
			if (argument == "-p"s)
			{
				try
				{
					if ((i + 1) <= argc)
					{
						std::string p(argv[i + 1]);
						port = std::stoi(p);
					}
				}
				catch (...)
				{
					std::cerr << "Неверно указан порт"s << std::endl;
					exit(1);
				}
			}
		}
	}

	//Считываем файл index.html
	if (std::filesystem::exists(index_file_dir))
	{
		std::ifstream file;
		file.open(canonical(index_file_dir).c_str(), std::ios::in);
		if (file.is_open())
		{
			file.seekg(0, std::ios::end); //указатель на конец файла
			size_t file_size = file.tellg(); //получаем размер файла
			file.seekg(0, std::ios::beg);
			index_page.resize(file_size);
			file.read(index_page.data(), file_size);
			file.close();
		}
		else
		{
			std::cerr << "Ошибка при открытии файла index.html"s << std::endl;
		}
	}
	else
	{
		std::cerr << "Путь к файлу index.html невалидный или нет доступа к нему.\nБудет использоваться встроенная страница index"s << std::endl;
	}

	//Считываем файл с хешом(sha256) пароля и устанавливаем флаг использования пароля
	if (std::filesystem::exists(pass_file_dir))
	{
		std::ifstream file;
		file.open(canonical(pass_file_dir).c_str(), std::ios::in);
		if (file.is_open())
		{
			file.seekg(0, std::ios::end); //указатель на конец файла
			size_t file_size = file.tellg(); //получаем размер файла
			file.seekg(0, std::ios::beg);
			pass_hash.resize(file_size);
			file.read(pass_hash.data(), file_size);
			file.close();
			pass_is_set = true;
		}
		else
		{
			std::cerr << "Ошибка при открытии файла pass.txt"s << std::endl;
		}
	}
	else
	{
		pass_is_set = false;
		std::cerr << "Путь к файлу pass.txt невалидный или нет доступа к нему.\nПароль не установлен."s << std::endl;
	}

	struct sockaddr_in server_addr;
	int socket_d;
	socket_d = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_d == -1) 
	{
		std::cerr << "Не удалось создать сокет"s << std::endl;
		return 1;
	}

	std::memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr.s_addr = INADDR_ANY;//INADDR_LOOPBACK | INADDR_ANY;

	if (bind(socket_d, reinterpret_cast<struct sockaddr*>(&server_addr), sizeof(server_addr)) == -1)
	{
		std::cerr << "Не удалось привязать сокет к адресу"s << std::endl;
		close(socket_d);
		return 1;
	}

	if (listen(socket_d, 2) == -1)
	{
		std::cerr << "Не удалось установить режим прослушивания"s << std::endl;
		close(socket_d);
		return 1;
	}

	std::cout << "Port: "s << port << std::endl;
	if (pass_is_set)
	{
		std::cout << "Пароль используестя."s << std::endl;
	}
	std::cout << "Программа ждет подключение...\n"s;

	while (true)
	{
		struct sockaddr_in client_addr;
		socklen_t client_addr_len = sizeof(client_addr);
		int client_d;
		client_d = accept(socket_d, reinterpret_cast<struct sockaddr*>(&client_addr), &client_addr_len);
		if (client_d == -1)
		{
			std::cerr << "Не удалось принять соединение"s << std::endl;
			close(socket_d);
			return 1;
		}

		char client_ip[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &(client_addr.sin_addr.s_addr), client_ip, INET_ADDRSTRLEN);
		std::cout << "Принято новое соединение от " << client_ip << ":" << ntohs(client_addr.sin_port) << std::endl;

		const size_t buf_size = 1024;
		char* buffer = new char[buf_size];
		ssize_t bytesRead = read(client_d, buffer, buf_size);
		std::string incoming_string(buffer, bytesRead);
		delete[] buffer;

		if (bytesRead == -1)
		{
			std::cerr << "Ошибка при чтении данных из сокета"s << std::endl;
			close(client_d);
			close(socket_d);
			return 1;
		}

		std::smatch match;
		std::string url_path;
		std::string password;
		if (std::regex_search(incoming_string, match, regexPathPass))
		{
			if(match[1].matched)
				url_path = match[1];
			if (match[2].matched && pass_is_set)
				password = match[2];
		}

		if (pass_is_set)
		{
			std::string hash_tmp = sha256(password);
			int res = strcasecmp(pass_hash.c_str(), hash_tmp.c_str());
			if (res != 0)
			{
				ssize_t bytesWritten = write(client_d, http_400.c_str(), http_400.size());
				if (bytesWritten == -1)
				{
					std::cerr << "Ошибка при записи данных в сокет"s << std::endl;
					//close(client_d);
					close(socket_d);
					return 1;
				}
				close(client_d);
				continue;
			}
		}

		if (url_path == "/"s)
		{
			std::string resp = http_ok;
			if (index_page.size() > 0)
			{
				resp += index_page;
			}
			else
			{
				resp += index_page_def;
			}
			ssize_t bytesWritten = write(client_d, resp.c_str(), resp.size());
			if (bytesWritten == -1)
			{
				std::cerr << "Ошибка при записи данных в сокет" << std::endl;
				close(client_d);
				close(socket_d);
				return 1;
			}
		}
		else if (url_path == "/shutdown"s)
		{
			std::string resp = http_ok + shutdown_str;
			ssize_t bytesWritten = write(client_d, resp.c_str(), resp.size());
			if (bytesWritten == -1)
			{
				std::cerr << "Ошибка при записи данных в сокет" << std::endl;
				close(client_d);
				close(socket_d);
				return 1;
			}
			std::cout << "Выключение..." << std::endl;
			system("shutdown now");
		}
		else if (url_path == "/reboot"s)
		{
			std::string resp = http_ok + reboot_str;
			ssize_t bytesWritten = write(client_d, resp.c_str(), resp.size());
			if (bytesWritten == -1)
			{
				std::cerr << "Ошибка при записи данных в сокет"s << std::endl;
				close(client_d);
				close(socket_d);
				return 1;
			}
			std::cout << "Перезагрузка..."s << std::endl;
			system("reboot");
		}
		close(client_d);
	}

	close(socket_d);
	std::cout << "Программа завершила свою работу"s << std::endl;

	return 0;
}


std::string sha256(const std::string str) {
	unsigned char hash[SHA256_DIGEST_LENGTH];

	SHA256_CTX sha256;
	SHA256_Init(&sha256);
	SHA256_Update(&sha256, str.c_str(), str.size());
	SHA256_Final(hash, &sha256);

	std::stringstream ss;

	for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
		ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
	}
	return ss.str();
}