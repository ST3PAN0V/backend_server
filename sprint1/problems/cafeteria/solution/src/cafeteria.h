#pragma once
#ifdef _WIN32
#include <sdkddkver.h>
#endif

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <memory>

#include "hotdog.h"
#include "result.h"

namespace net = boost::asio;

using namespace std::literals;
// Функция-обработчик операции приготовления хот-дога
using HotDogHandler = std::function<void(Result<HotDog> hot_dog)>;


class OrderHandler : public std::enable_shared_from_this<OrderHandler> {
public:
    OrderHandler(HotDogHandler handler, net::io_context &io, int id_hotdog, Store& store, std::shared_ptr<GasCooker> gas_cooker )
        : handler_(handler), io_(io), id_hotdog_(id_hotdog), store_(store), gas_cooker_(gas_cooker) {}

    void Run() {
        auto bread = store_.GetBread();
        auto sausage = store_.GetSausage();
        
        net::defer(io_, [self = shared_from_this(), sausage = sausage->shared_from_this(), bread = bread->shared_from_this(), gas_cooker_ = gas_cooker_->shared_from_this()](){
            bread->StartBake(*gas_cooker_->shared_from_this(), [sausage = sausage->shared_from_this(), bread = bread->shared_from_this(), self = self->shared_from_this()](){
                auto bread_timer_ = std::make_shared<net::steady_timer>(self->io_, std::chrono::milliseconds(1002));
                bread_timer_->async_wait([timer = bread_timer_, sausage = sausage->shared_from_this(), bread = bread->shared_from_this(), self = self->shared_from_this()](const boost::system::error_code& error){
                    bread->StopBaking();
                    self->MakeHotDog(sausage->shared_from_this(), bread->shared_from_this(), self->handler_);
                });
            });

            sausage->StartFry(*gas_cooker_->shared_from_this(), [sausage = sausage->shared_from_this(), bread = bread->shared_from_this(), self = self->shared_from_this()](){
                auto sausage_timer_ = std::make_shared<net::steady_timer>(self->io_, std::chrono::milliseconds(1502));
                sausage_timer_->async_wait([timer = sausage_timer_, sausage = sausage->shared_from_this(), bread = bread->shared_from_this(), self = self->shared_from_this()](const boost::system::error_code& error){
                    sausage->StopFry();
                    self->MakeHotDog(sausage->shared_from_this(), bread->shared_from_this(), self->handler_);
                });
            });
        });
    }

    void MakeHotDog(std::shared_ptr<Sausage> sausage, std::shared_ptr<Bread> bread, HotDogHandler handler) {
        try{
            if (sausage->IsCooked() && bread->IsCooked()) {
                auto hotdog = HotDog(id_hotdog_, sausage, bread);
                auto preres = Result<HotDog>(HotDog(id_hotdog_, sausage->shared_from_this(), bread->shared_from_this()));
                auto res = std::make_shared<Result<HotDog>>(preres);
                handler(*res.get());
                sausage.reset();
                bread.reset();
                return;
            }
        } catch (const std::exception& ex) {
            std::cout << ex.what() << std::endl;
        }
    }

private:
    net::io_context& io_;
    //net::steady_timer sausage_timer_{io_, std::chrono::milliseconds(1502)}; 
    //net::steady_timer bread_timer_{io_, std::chrono::milliseconds(1002)};
    HotDogHandler handler_;
    int id_hotdog_;
    Store& store_;
    std::shared_ptr<GasCooker> gas_cooker_;
};

// Класс "Кафетерий". Готовит хот-доги
class Cafeteria {
public:
    
    explicit Cafeteria(net::io_context& io)
        : io_{io} {
}

    // Асинхронно готовит хот-дог и вызывает handler, как только хот-дог будет готов.
    // Этот метод может быть вызван из произвольного потока
    void OrderHotDog(HotDogHandler handler) {
        const int order_id = ++id_hotdog;
        std::make_shared<OrderHandler>(handler, io_, order_id, store_, gas_cooker_->shared_from_this())->Run();
    }


private:

    net::io_context& io_;
    int id_hotdog = 0;
    // Используется для создания ингредиентов хот-дога
    Store store_;
    // Газовая плита. По условию задачи в кафетерии есть только одна газовая плита на 8 горелок
    // Используйте её для приготовления ингредиентов хот-дога.
    // Плита создаётся с помощью make_shared, так как GasCooker унаследован от
    // enable_shared_from_this.
    std::shared_ptr<GasCooker> gas_cooker_ = std::make_shared<GasCooker>(io_);
};

