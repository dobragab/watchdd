#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <algorithm>
#include <signal.h>
#include <unistd.h>

using std::string;

std::vector<int> get_process_id_by_name(string name)
{
    constexpr int bufsize_pgrep = 32;
    char buf[bufsize_pgrep];
    std::vector<int> result;
    string command = string("pgrep -x ") + name;
    FILE * fp = popen(command.c_str(), "r");

    while(fgets(buf, bufsize_pgrep, fp) != nullptr)
    {
        int pid = atoi(buf);
        result.push_back(pid);
    }

    pclose(fp);

    return result;
}

int interval = 5;
bool daemon_mode = false;
int process_id = 0;
std::vector<int> processes;

void parse_params(int argc, char ** argv)
{
    std::vector<string> params(argc-1);

    for (int i = 1; i < argc; ++i)
        params.push_back(argv[i]);

    for(unsigned i = 0; i < params.size(); ++i)
    {
        std::string& p = params[i];

        if (p.size() == 0)
            continue;

        if (p[0] != '-' || p.size() != 2)
        {
            std::cout << "Invalid parameter: " << p << '#' << std::endl;
            continue;
        }

        char c = p[1];
        switch(c)
        {
            case 'd':
                daemon_mode = true;
                break;
            case 'i':
            {
                int temp;
                if (i == params.size() - 1 || (temp = atoi(params[i+1].c_str())) <= 0)
                    std::cout << "Invalid usage of -i" << std::endl;
                else
                {
                    interval = temp;
                    ++i;
                }
                break;
            }
            case 'p':
            {
                int temp;
                if (i == params.size() - 1 || (temp = atoi(params[i+1].c_str())) <= 0)
                    std::cout << "Invalid usage of -p" << std::endl;
                else
                {
                    process_id = temp;
                    ++i;
                }
                break;
            }
            default:
                std::cout << "Invalid parameter: " << p << std::endl;
                break;
        }
    }
}

template<typename T>
void remove_vec(std::vector<T>& vec, T const& value)
{
    vec.erase(std::remove(vec.begin(), vec.end(), value), vec.end());
}

void _kill(int pid)
{
    if (kill(pid, SIGUSR1) == 0) // no error
        return;

    switch(errno)
    {
        case EPERM:
            std::cout << "Access denied on PID " << pid << std::endl;
            remove_vec(processes, pid);
            break;
        case ESRCH:
            std::cout << "PID " << pid << " does not exist." << std::endl;
            remove_vec(processes, pid);
            break;
        case EINVAL:
            std::cout << "Signal USR1 is unknown. ???" << std::endl;
            break;
        default:
            std::cout << "An unknown error occured." << std::endl;
            break;
    }
}


int main(int argc, char ** argv)
{
    parse_params(argc, argv);

    std::cout << "Watching dd..." << std::endl;

    if (process_id == 0)
        processes = get_process_id_by_name("dd");
    else
        processes.push_back(process_id);

    bool quit = false;

    while(!quit)
    {
        if (daemon_mode && process_id == 0)
            processes = get_process_id_by_name("dd");

        if(process_id == 0)
            for (int pid : processes)
                _kill(pid);
        else
            _kill(process_id);

        if (!daemon_mode && processes.size() == 0)
            quit = true;

        sleep(interval);
    }

    return 0;
}
