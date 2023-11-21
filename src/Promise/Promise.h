//
// Created by damdi on 19.11.2023.
//

#ifndef COROUTINESC_20_TEST_PROMISE_H
#define COROUTINESC_20_TEST_PROMISE_H
#include <coroutine>
#include <variant>
#include <exception>
#include <concepts>
template<class PromiseType>
struct Task;

template<class PromiseType>
struct Promise {
    Task<PromiseType> get_return_object() noexcept {
        return Task<PromiseType>{ this };
    }
    std::suspend_never initial_suspend() noexcept {return {};}
    std::suspend_always final_suspend() noexcept {return {};}
    template<class U>
    void return_value(U&& value)
    requires std::is_nothrow_constructible_v<PromiseType, decltype(std::forward<U>(value))>
    {
        result.template emplace<1>(std::forward<U>(value));
    }

    void unhandled_exception()
    requires std::is_nothrow_assignable<std::exception_ptr,std::exception_ptr>::value
    {
        result.template emplace<2>(std::current_exception());
    }

    [[nodiscard]] bool isReady()const noexcept{
        return result.index() != 0;
    }

    PromiseType&& getResult(){
        if(result.index() == 2){
            std::rethrow_exception(std::get<2>(result));
        }
        return std::move(std::get<1>(result));
    }

    std::variant<std::monostate,PromiseType,std::exception_ptr> result;
    std::coroutine_handle<> continuation;
};

template<class T>
struct [[nodiscard]] Task{
    using promise_type = Promise<T>;
    Task() = default;
    auto operator co_await() const noexcept{
        struct Awaitable{
            [[nodiscard]] bool await_ready()const noexcept{
                return promise_.isReady();
            }
            using CoroHandle = std::coroutine_handle<>;
            [[nodiscard]] CoroHandle await_suspend(CoroHandle continuation)const noexcept{
                promise_.continuation = continuation;
                return std::coroutine_handle<Promise<T>>::from_promise(promise_);
            }
            T&& await_resume()const{
                return promise_.getResult();
            }

            auto final_suspend(){
                struct FinalAwaitable{
                    [[nodiscard]] bool await_ready()const noexcept{return false;}
                    void await_suspend(std::coroutine_handle<Promise<T>> thisCoro){
                        auto& promise = thisCoro.promise();
                        if(promise.continuation)
                            promise.continuation();
                    }
                    void await_resume()const noexcept{}
                };
                return FinalAwaitable{};
            }

            Promise<T>& promise_;
        };
        return Awaitable{*promise_};
    }

private:
    explicit Task(Promise<T>* promise):promise_(promise){}

    Promise<T>* promise_;
    template<class>
    friend struct Promise;
};


#endif //COROUTINESC_20_TEST_PROMISE_H
