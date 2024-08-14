#include <sstream>
#include <iomanip>


#include "player_tokens.h"

namespace app 
{    
    Player* PlayerTokens::FindPlayerByToken(const Token token)
    {
        auto it = token_to_player_.find(token);
        
        if(token_to_player_.end() == it)
        {
            return nullptr;
        }
        
        return it->second;
    }
    

    Token PlayerTokens::AddPlayer(Player* player)
    {
        //https://stackoverflow.com/questions/5100718/integer-to-hex-string-in-c
        /*std::stringstream stream;
        stream << std::hex << your_int;
        std::string result( stream.str() );*/
        
        /*stream << std::setfill ('0') << std::setw(sizeof(your_type)*2) 
               << std::hex << your_int;*/
        
        /*
        template< typename T >
        std::string int_to_hex( T i )
        {
          std::stringstream stream;
          stream << "0x" 
                 << std::setfill ('0') << std::setw(sizeof(T)*2) 
                 << std::hex << i;
          return stream.str();
        }*/
        const uint64_t first{generator1_()};
        const uint64_t second{generator2_()};
        
        std::stringstream stream;
        stream <<std::setfill ('0')<< std::hex<< std::setw(sizeof(uint64_t)*2)<< first
               <<std::setfill ('0')<< std::hex<< std::setw(sizeof(uint64_t)*2)<< second;
        
        Token out{stream.str()};
        
        token_to_player_[out] = player;
        return out;
    }
     
}
