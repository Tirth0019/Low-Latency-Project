Week-1 Plan:

Pre-Requisite to Learn:
-Modern C++ Topics
 -RAII
 -Move Semantics
 -std::chrono
 -inline function

Week-1: What to Implement (Corrected)

 Order Representation
    -Implement in include/ first:
    -OrderId
    -Timestamp
    -Side { Buy, Sell }
    -Price
    -Quantity

 Order Logic (Single-Threaded)

    -Implement in src/:
    -Add order
    -Cancel order
    -Modify order (optional but good)
    -Match orders
    -Partial fills
    -Maintain best bid / ask

    Use simple containers first:

        std::map
        std::deque