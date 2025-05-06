#include <iostream>
#include <vector>
#include <set>
#include <random>
#include <algorithm>
#include <chrono>
#include <thread>
#include <mutex>
#include <numeric>

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
                    if (rand < 0.005 * total_5star * (i / (double)total_5star)) {
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
                    if (rand < 0.0075 * total_4star * (i / (double)total_4star)) {
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
                    if (rand < 0.0075 * total_4star * (i / (double)total_4star)) {
                        cards_4star.insert(i);
                        break;
                    }
                }
            }
        }
        next_draw:

        // ��ѡ�߼�
        // if (draws % 300 == 200) {
        //     if (cards_5star.size() < want_5star) {
        //         for (int i = 1; i <= want_5star; i++) {
        //             if (cards_5star.find(i) == cards_5star.end()) {
        //                 cards_5star.insert(i);
        //                 goto skip_4star_choice;
        //             }
        //         }
        //     }
        //     if (cards_4star.size() < want_4star) {
        //         for (int i = 1; i <= want_4star; i++) {
        //             if (cards_4star.find(i) == cards_4star.end()) {
        //                 cards_4star.insert(i);
        //                 break;
        //             }
        //         }
        //     }
        //     skip_4star_choice:;
        // }
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
    double neineigeneinei = draw_counts.back();
    
    std::cout << "----------------ģ����----------------" << std::endl;
    std::cout << "�����鿨����: " << expected_draws << std::endl;
    std::cout << "��λ���鿨����: " << percentile_50 << "����" << percentile_50 /40  << "w��ʯ" << std::endl;
    std::cout << "90%��������³����ڼ���: " << percentile_90 << "����" << percentile_90 /40 << "w��ʯ" << std::endl;
    std::cout << "��������鿨����: " << neineigeneinei << "����" << neineigeneinei /40 << "w��ʯ" << std::endl;
    // ������ʱ�������ʱ
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    double seconds = duration.count() / 1000.0;
    
    std::cout << "ģ���ʱ: " << seconds << " ��" << std::endl;

    if(reverseFlag == 1) {
        double input, sigma, z, cdfValue;
        std::cout << "��������ʹ�õĳ�����";
        std::cin >> input;
        if(input < 10) {
            std::cerr << "����ֵ��С��" << std::endl;
            return -1;
        }
        sigma = (percentile_90 - expected_draws)/1.28155;
        //sigma = std::sqrt(0.005 * simulations); //�˼���ʱ�µĽ��Ʒ�ʽ��Ȼ�����Լ�������
        z = (input - expected_draws)/sigma;
        cdfValue = 0.5 * (1 + std::erf(z / std::sqrt(2)));
        std::cout << "����ֵ " << input << " ��Ӧ�ۻ�����ԼΪ " << cdfValue * 100.0 << "% ��" << std::endl;
    }
    return 0;
}

void printhelp() {
    std::cout << "Options: \n"
                << "  --reverse    -r    ���Ƴ�������\n"
                << "  --number     -n    ָ��ģ���������Ӧ����100���\n"
                << "  --version    -v    ��ʾ�汾��Ϣ\n"
                << "  --help       -h    ��ʾ����" <<std::endl;
}

int main(int argc, char* argv[]) {
    int total_5star = 0, want_5star = 0;
    int total_4star = 0, want_4star = 0;
    int isNormal = 1, simulations = 1000000;
    //unsigned int user_threads = 8; //skip threads select
    bool reverseFlag = 0;

    // �ֶ���������
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        // std::cout << arg <<std::endl;
        if (arg == "--reverse" || arg == "-r") {
           reverseFlag = 1;
           std::cout << "��ǰ���ڷ��Ƴ�������ģʽ������ڱ��׻��Ƶ�ԭ�򣬳���ʹ�ò����ϸ���ѭ��̬�ֲ�����������ο�" <<std::endl;
           } else if (arg == "--version" || arg == "-v") {
               std::cout << "Version 1.7,Build 35 \n"
                   << "Copyright (c) 2025, ɽ����Ҷ����Modified by UDMH \n"
                   << "Original page at: https://gitee.com/handsome-druid/bangdream-gacha" << std::endl;
               return 0;
           } else if (arg == "--number" || arg == "-n") {
               i++;
               int tmpSimulations = std::atoi(argv[i]);
               if(tmpSimulations > simulations){
                   simulations = tmpSimulations;
                   }
                   else {
                       std::cout << "��ʹ��Ĭ��ֵ" << std::endl;
                   }
           }
           else if (arg == "--help" || arg == "-h") {
               printhelp();
               return 0;
           } else {
               std::cerr << "δ֪����: " << arg << std::endl;
               return -1;
           }
       }

    std::cout << "��ӭʹ�ó鿨����ģ������modified ver1.7" << std::endl;
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

    // ��main�������޸��߳�����ⲿ��
    unsigned int thread_count = std::thread::hardware_concurrency();
    if (thread_count == 0) thread_count = 4;

    std::cout << "����ʹ�õ��߳�����-1�鿴����˵����0ʹ�ñ��ؽ���ֵ����";
    unsigned int user_threads;
    std::cin >> user_threads;
    if (user_threads == -1) {
        std::cout << "----------------------------------------" << std::endl;
        std::cout << "��⵽ϵͳ֧�ֵĲ����߳�����" << thread_count << std::endl;
        std::cout << "ע�⣺�����ʹ�õ���Intel��С�˴�������12������£���" << std::endl;
        std::cout << "1. �����ʹ��P�����������߳�" << std::endl;
        std::cout << "2. ����ͨ�����������->����->CPU->��������ϸ��Ϣ�鿴P��������" << std::endl;
        std::cout << "��������Ҫʹ�õ��߳�����1-" << thread_count << "��\n";
        std::cout << "����ֵ��" << std::max(1u, thread_count / 4) << "-" << std::max(1u, thread_count / 2) 
                 << " ������CPU�ܹ�ѡ����ʵ�ֵ��\n";
        std::cout << "����0��ʹ�ñ��ؽ���ֵ��"<< std::max(1u, thread_count / 4 )<< "����";
        std::cin >> user_threads;
    }

    if (user_threads == 0) {
        // ʹ�ø����ص�Ĭ��ֵ�������ڴ�С�˼ܹ���ʹ�ù����߳�
        user_threads = std::max(1u, thread_count / 4);
        std::cout << "ʹ�ñ��ؽ���ֵ��" << user_threads << " �߳�\n";
    } else if (user_threads > thread_count) {
        std::cout << "���棺ָ�����߳�������ϵͳ֧�ֵ����ֵ����ʹ��" << thread_count << "�߳�\n";
        user_threads = thread_count;
    } else if (user_threads < 1) {
        std::cout << "�߳����������0����ʹ��1���߳�\n";
        user_threads = 1;
    }

    calculate_statistics(total_5star, want_5star, total_4star, want_4star, isNormal, simulations, user_threads, reverseFlag);
    std::cout << "\n���س����˳�����...";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::cin.get();
    return 0;
}
