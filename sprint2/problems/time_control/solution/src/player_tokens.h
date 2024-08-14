#pragma once



#include <random>
#include <unordered_map>

#include "tagged.h"

#include "player.h"

namespace app 
{
    namespace detail 
    {
        struct TokenTag {};
    }  // namespace detail
    
    using Token = util::Tagged<std::string, detail::TokenTag>;
    
    class PlayerTokens 
    {
        public:
            PlayerTokens() = default;
            
            Player* FindPlayerByToken(const Token token);
            Token AddPlayer(Player* player);
            
        private:
            using TokenHasher = util::TaggedHasher<Token>;
            std::random_device random_device_;
            std::mt19937_64 generator1_{[this] {
                std::uniform_int_distribution<std::mt19937_64::result_type> dist;
                return dist(random_device_);
            }()};
            std::mt19937_64 generator2_{[this] {
                std::uniform_int_distribution<std::mt19937_64::result_type> dist;
                return dist(random_device_);
            }()};
            
            std::unordered_map<Token, Player*, TokenHasher> token_to_player_;
            
            // Чтобы сгенерировать токен, получите из generator1_ и generator2_
            // два 64-разрядных числа и, переведя их в hex-строки, склейте в одну.
            // Вы можете поэкспериментировать с алгоритмом генерирования токенов,
            // чтобы сделать их подбор ещё более затруднительным
            
            
            
            
    }; 
}



