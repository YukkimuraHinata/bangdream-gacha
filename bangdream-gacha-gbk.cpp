#include <iostream>
#include <vector>
#include <set>
#include <random>
#include <algorithm>
#include <chrono>
#include <thread>
#include <mutex>
#include <numeric>
#include "color.h"

#define Simulations 1000000 //����ģ�����Ϊһ�����

//����һ��������������࣬����ģ��鿨
class GachaRandom {
private:
    std::random_device rd;
    std::mt19937 gen;
    std::uniform_real_distribution<> dis;
    std::uniform_int_distribution<> dis_5star;

public:
    GachaRandom(int total_5star) 
        : gen(rd())
        , dis(0.0, 1.0)
        , dis_5star(1, total_5star) 
    {}

    // ��ȡ0-1֮��������
    double get_random() {
        return dis(gen);
    }

    // ��ȡ5�ǿ���������
    int get_5star_random() {
        return dis_5star(gen);
    }
};

typedef struct arg_processing_return {
    bool reverse_flag = false;
    bool unknow_arg = false;
    bool need_to_exit = false;
    int simulations = Simulations;
    unsigned int threads = 1;
} apr;

int simulate_one_round(int total_5star, int want_5star, int total_4star, int want_4star, int normal, GachaRandom& random) {
    std::set<int> cards_5star;  // ��ӵ�е�5�ǿ�Ƭ����
    std::set<int> cards_4star;  // ��ӵ�е�4�ǿ�Ƭ����
    int draws = 0;  // �Ѿ��鿨����
    int choose_times_have = 0; // ӵ�е���ѡ����

    while (true) {
        draws++;
        // �޸�50�����߼�
        if (draws % 50 == 0 && normal == 1) {
            if (want_5star > 0) {  // ֻ�е�������Ҫ5�ǿ�ʱ�ſ���
                int roll = random.get_5star_random();  // �����ȡһ��5�ǿ�
                if (roll <= want_5star) {  // ����鵽������Ҫ�Ŀ�
                    cards_5star.insert(roll);
                }
            }
            goto next_draw;  // 50���ױض���5�ǣ�����4���ж�
        }
        else {
            // �����鿨
            double rand = random.get_random();
            // ���ж�5��
            if (want_5star > 0) {  // ֻ�е�������Ҫ5�ǿ�ʱ���ж�
                for (int i = 1; i <= want_5star; i++) {
                    if (rand < 0.005 * i) {
                        cards_5star.insert(i);
                        goto next_draw;
                    }
                }
            }

            // �����Ҫ��5������Ϊ0����ض���Ҫ4��
            //���ƶ��һ�ٵ�һ��else��֧������Ϊ���߳�100���ģ����̴�Լһ��ļ���ʱ�䣨�Ժ���
            else {
                rand = random.get_random();
                for (int i = 1; i <= want_4star; i++) {
                    if(rand < 0.0075 * i) {
                        cards_4star.insert(i);
                        break;
                    }
                }
                goto next_draw;  // �����ظ���4���ж�
            }

            // ���ж�4��
            if (total_4star > 0 && want_4star > 0) {
                rand = random.get_random();
                for (int i = 1; i <= want_4star; i++) {
                    if(rand < 0.0075 * i) {
                        cards_4star.insert(i);
                        break;
                    }
                }
            }
        }
        next_draw:

        //�µ���ѡ�߼����Ȱѳ鿨���꣬�ٽ�����ѡ�������ظ��鵽
        choose_times_have = ( draws + 100 ) / 300;
        if(cards_5star.size() + cards_4star.size() + choose_times_have >= want_5star + want_4star) {
            break; 
        }
    }
    
    return draws;
}

// �޸�ͳ�ƺ���ǩ��������߳�������
int calculate_statistics(int total_5star, int want_5star, int total_4star, int want_4star, int normal, 
                        int simulations, unsigned int thread_count, bool reverseFlag) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    std::cout << "----------------�������----------------" << std::endl;
    std::cout << "����5������: " << total_5star << std::endl;
    std::cout << "��Ҫ�ĵ���5������: " << want_5star << std::endl;
    std::cout << "��Ҫ�ĵ���4������: " << want_4star << std::endl;
    std::cout << "50С����: " << (normal == 1 ? "��" : "��") << std::endl;
    std::cout << "ģ�����: " << simulations << std::endl;
    std::cout << "ʹ���߳���: " << thread_count << std::endl;
    std::cout << "������..." << std::endl;

    std::vector<int> draw_counts;
    draw_counts.reserve(simulations);
    std::mutex mtx;
    long long total_draws = 0;
    
    // ʹ���û�ָ�����߳���
    std::vector<std::thread> threads;
    int sims_per_thread = simulations / thread_count;
        
    // ��������߳�ִ��ģ��
    for (unsigned int i = 0; i < thread_count; ++i) {
        threads.emplace_back([&, i]() {
            GachaRandom random(total_5star);  // ÿ���߳�ʹ�ö����������������
            std::vector<int> local_draws;     // �̱߳��ش洢
            local_draws.reserve(sims_per_thread);
            
            int start = i * sims_per_thread;
            int end = (i == thread_count - 1) ? simulations : (i + 1) * sims_per_thread;
            
            for (int j = start; j < end; ++j) {
                int draws = simulate_one_round(total_5star, want_5star, total_4star, want_4star, normal, random);
                local_draws.push_back(draws);
            }
            
            // �ϲ����
            std::lock_guard<std::mutex> lock(mtx);
            draw_counts.insert(draw_counts.end(), local_draws.begin(), local_draws.end());
        });
    }
    
    // �ȴ������߳����
    for (auto& thread : threads) {
        thread.join();
    }
    
    // �����ܳ���
    total_draws = std::accumulate(draw_counts.begin(), draw_counts.end(), 0LL);
    
    double expected_draws = static_cast<double>(total_draws) / simulations;
    std::sort(draw_counts.begin(), draw_counts.end());
    double percentile_50 = draw_counts[simulations / 2];
    double percentile_90 = draw_counts[static_cast<int>(simulations * 0.9)];
    double max_number = draw_counts.back();
    
    std::cout << "----------------ģ����----------------" << std::endl;
    std::cout << "�����鿨����: " << ANSI_Cyan << expected_draws << ANSI_COLOR_RESET << std::endl;
    std::cout << "��λ���鿨����: " << ANSI_Cyan << percentile_50 << ANSI_COLOR_RESET << "����" << percentile_50 /40  << "w��ʯ" << std::endl;
    std::cout << "90%��������³����ڼ���: " << ANSI_Cyan << percentile_90 << ANSI_COLOR_RESET << "����" << percentile_90 /40 << "w��ʯ" << std::endl;
    std::cout << "��������鿨����: " << ANSI_Cyan << max_number << ANSI_COLOR_RESET << "����" << max_number /40 << "w��ʯ" << std::endl;
    // ������ʱ�������ʱ
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    double seconds = duration.count() / 1000.0;
    
    std::cout << "ģ���ʱ: " << seconds << " ��" << std::endl;

    if(reverseFlag) {
        double input, sigma, z, cdfValue;
        std::cout << "��������ʹ�õĳ�����";
        std::cin >> input;
        if(input < 10) {
            std::cerr << "����ֵ��С��" << std::endl;
            return -1;
        } else if(input > max_number) {
            std::cout << "�����ǻ������ˣ�" << std::endl;
            return -1;
        } else {
            sigma = (percentile_90 - expected_draws)/1.28155;
            z = (input - expected_draws)/sigma;
            cdfValue = 0.5 * (1 + std::erf(z / 1.41421)); //sqrt(2)
            std::cout << "����ֵ " << input << " ��Ӧ�ۻ�����ԼΪ " << ANSI_Cyan << cdfValue * 100.0 << "% " << ANSI_COLOR_RESET << std::endl;
        }
    }
    return 0;
}

inline apr arg_processing(int argc, const char* argv[]) {
    apr Result;
    unsigned int thread_count = std::thread::hardware_concurrency();
    if (thread_count == 0) {
        thread_count = 4;
    }
    Result.threads = std::max(1u,thread_count / 2u);
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--reverse" || arg == "-r") {
            Result.reverse_flag = true;
            std::cout << ANSI_Red <<"��ǰ���ڷ��Ƴ�������ģʽ����������ο�" << ANSI_COLOR_RESET << std::endl;
            } else if (arg == "--thread" || arg == "-t") {
                i++;
                int user_threads;
                user_threads = std::atoi(argv[i]);
                if(user_threads < 1) {
                    std::cout << ANSI_Red_BG << "�߳����������0����ʹ��1���߳�" << ANSI_COLOR_RESET <<std::endl;
                    Result.threads = 1u;
                } else if (user_threads > thread_count) {
                std::cout << ANSI_Red_BG << "���棺ָ�����߳�������CPU�̣߳���ʹ��" << thread_count << "�߳�" << ANSI_COLOR_RESET <<std::endl;
                Result.threads = thread_count;
                } else {
                Result.threads = (unsigned int)user_threads;
                }
            } else if (arg == "--version" || arg == "-v") {
                std::cout << "BanG Dream! Gacha,version 1.9.1,Build 54 \n"
                    << "Copyright (c) 2025, ɽ����Ҷ����Modified by UDMH \n"
                    << "Original page at: https://gitee.com/handsome-druid/bangdream-gacha \n"
                    << "My GitHub page at: https://github.com/YukkimuraHinata/bangdream-gacha \n"
                    << "����ʱ��: " << __DATE__  << " " << __TIME__ << "\n"
                    << "C++ Version: " << __cplusplus << "\n" << std::endl;
                Result.need_to_exit = true;
            } else if (arg == "--number" || arg == "-n") {
                i++;
                int tmpSimulations = std::atoi(argv[i]);
                if(tmpSimulations > Simulations){
                    Result.simulations = tmpSimulations;
                    } else {
                        std::cout << "��ʹ��Ĭ��ֵ:" << Simulations << std::endl;
                    }
            } else if (arg == "--help" || arg == "-h") {
                std::cout << "Options: \n"
                << "  --reverse    -r    ���Ƴ�������\n"
                << "  --number     -n    ָ��ģ���������Ӧ����100���\n"
                << "  --thread     -t    ָ��ʹ�õ��߳�����������CPU���߳�\n"
                << "  --version    -v    ��ʾ�汾��Ϣ\n"
                << "  --help       -h    ��ʾ����\n" <<std::endl;
                Result.need_to_exit = true;
            } else {
                std::cerr << "δ֪����: " << ANSI_Red << arg << ANSI_COLOR_RESET << std::endl;
                std::cout << "���롰bangdream-gacha -h���鿴����" << std::endl;
                Result.unknow_arg = true;
                Result.need_to_exit = true;
                break;
            }
        }
    return Result;
}

int main(int argc, char* argv[]) {
    std::ios::sync_with_stdio(false);
    apr res = arg_processing(argc, const_cast<const char**>(argv));
    if(res.need_to_exit) {
        return res.unknow_arg;
    }
    int isNormal = 1;
    int total_5star = 0, want_5star = 0, total_4star = 0, want_4star = 0;
    std::cout << ANSI_Blue_BG << "BanG Dream! Gacha,a gacha simulator of Garupa" << ANSI_COLOR_RESET << std::endl;
    std::cout << "�����뵱��5�ǿ���������: ";
    std::cin >> total_5star;
    if (total_5star < 0) {
        std::cout << "��Ƭ����������ڵ���0��" << std::endl;
        return 1;
    }

    if (total_5star > 0) {
        std::cout << "��������Ҫ��ȡ�ĵ���5�ǿ�����: ";
        std::cin >> want_5star;
        if (want_5star < 0 || want_5star > total_5star) {
            std::cout << "��Ҫ�Ŀ�Ƭ����������0��" << total_5star << "֮�䣡" << std::endl;
            return 1;
        }
    }

        std::cout << "��������Ҫ��ȡ�ĵ���4�ǿ�����: ";
        std::cin >> want_4star;
        if (want_4star < 0) {
            std::cout << "����С��0��" << std::endl;
            return 1;
        }
    total_4star = want_4star;
    std::cout << "�Ƿ�Ϊ��פ�أ��Ƿ���50С���ף���1: �ǣ�0: ��: ";
    std::cin >> isNormal;
    if (isNormal != 0 && isNormal != 1) {
        std::cout << "������󣡣�1: �ǣ�0: ��" << std::endl;
        return 1;
    }

    int return_value = calculate_statistics(total_5star, want_5star, total_4star, want_4star, isNormal, 
                        res.simulations, res.threads, res.reverse_flag);

    std::cout << "\n���س����˳�����...";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cin.get();

    return return_value;
}
