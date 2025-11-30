#pragma once

// https://en.cppreference.com/w/cpp/utility/variant/visit
template <class... Ts>
struct LambdaVisitor : Ts... {
    using Ts::operator()...;
};

template <class... Ts>
LambdaVisitor(Ts...) -> LambdaVisitor<Ts...>;
