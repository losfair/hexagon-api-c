#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <vector>
#include "ort.h"
#include "ort_assembly_writer.h"

using namespace hexagon;

ort::Function build_call_tester() {
    using namespace assembly_writer;

    FunctionWriter fwriter;

    BasicBlockWriter init_bb;
    init_bb
        .Write(BytecodeOp("LoadNull"))
        .Write(BytecodeOp("LoadString", Operand::String("set_ret")))
        .Write(BytecodeOp("GetStatic"))
        .Write(BytecodeOp("Call", Operand::I64(0)))
        .Write(BytecodeOp("Return"));

    fwriter.Write(init_bb);
    return fwriter.Build();
}

ort::Function build_call_tester_with_callee_as_param() {
    using namespace assembly_writer;
    
    FunctionWriter fwriter;

    BasicBlockWriter init_bb;
    init_bb
        .Write(BytecodeOp("LoadNull"))
        .Write(BytecodeOp("GetArgument", Operand::I64(0)))
        .Write(BytecodeOp("Call", Operand::I64(0)))
        .Write(BytecodeOp("Return"));

    fwriter.Write(init_bb);
    return fwriter.Build();
}

ort::Function build_sum_tester() {
    using namespace assembly_writer;

    FunctionWriter fwriter;

    fwriter.Write(
        BasicBlockWriter()
            .Write(BytecodeOp("InitLocal", Operand::I64(3)))
            .Write(BytecodeOp("GetArgument", Operand::I64(0)))
            .Write(BytecodeOp("SetLocal", Operand::I64(0)))
            .Write(BytecodeOp("GetArgument", Operand::I64(1)))
            .Write(BytecodeOp("SetLocal", Operand::I64(1)))
            .Write(BytecodeOp("LoadInt", Operand::I64(0)))
            .Write(BytecodeOp("SetLocal", Operand::I64(2)))
            .Write(BytecodeOp("Branch", Operand::I64(1)))
    ).Write(
        BasicBlockWriter()
            .Write(BytecodeOp("GetLocal", Operand::I64(1)))
            .Write(BytecodeOp("GetLocal", Operand::I64(0)))
            .Write(BytecodeOp("TestLt"))
            .Write(BytecodeOp("ConditionalBranch", Operand::I64(2), Operand::I64(3)))
    ).Write(
        BasicBlockWriter()
            .Write(BytecodeOp("LoadInt", Operand::I64(1)))
            .Write(BytecodeOp("GetLocal", Operand::I64(0)))
            .Write(BytecodeOp("IntAdd"))
            .Write(BytecodeOp("Dup"))
            .Write(BytecodeOp("SetLocal", Operand::I64(0)))
            .Write(BytecodeOp("GetLocal", Operand::I64(2)))
            .Write(BytecodeOp("IntAdd"))
            .Write(BytecodeOp("SetLocal", Operand::I64(2)))
            .Write(BytecodeOp("Branch", Operand::I64(1)))
    ).Write(
        BasicBlockWriter()
            .Write(BytecodeOp("GetLocal", Operand::I64(2)))
            .Write(BytecodeOp("Return"))
    );

    return fwriter.Build();
}

void bench(const char *name, const std::function<void (int n)>& cb) {
    const int n = 1000000;

    printf("Bench: %s\n", name);
    clock_t start_time = clock();

    cb(n);

    clock_t end_time = clock();
    unsigned long long elapsed_ns = ((end_time - start_time) / (CLOCKS_PER_SEC / 1000)) * 1000000;

    printf("Done. %llu ns / iter\n", elapsed_ns / (unsigned long long) n);
}

void test_sum() {
    ort::Function f = build_sum_tester();
    ort::Runtime rt;
    rt.AttachFunction(
        "sum",
        f
    );
    ort::Value entry = rt.GetStaticObject("sum");

    long long val = 0;

    bench("sum", [&](int n) {
        std::vector<ort::Value> params;
        params.push_back(ort::Value::FromInt(0));
        params.push_back(ort::Value::FromInt(n));

        ort::Value ret = rt.Invoke(entry, params);
        val = ret.ExtractI64();
    });

    printf("%lld\n", val);
}

class Adder : public ort::ProxiedObject {
public:
    virtual ort::Value Call(const std::vector<ort::Value>& args) {
        return ort::Value::FromInt(args.at(0).ExtractI64() + args.at(1).ExtractI64());
    }
};

void test_object_handle() {
    ort::Runtime rt;
    ort::Value val = ort::Value::FromString("Hello world", rt);
    bench("object_handle", [&](int n) {
        for(int i = 0; i < n; i++) {
            ort::ObjectHandle handle = val.ToObjectHandle(rt);
        }
    });
}

void test_proxied() {
    using namespace assembly_writer;

    ort::Runtime rt;

    ort::Function po_feed = ort::Function::LoadNative([&rt]() {
        ort::ObjectProxy proxy(new Adder());
        return proxy.Pin(rt);
    });

    rt.AttachFunction("po_feed", po_feed);

    ort::Function vcaller = FunctionWriter()
        .Write(
            BasicBlockWriter()
                .Write(BytecodeOp("LoadInt", Operand::I64(1)))
                .Write(BytecodeOp("LoadInt", Operand::I64(2)))
                .Write(BytecodeOp("LoadNull"))
                .Write(BytecodeOp("LoadNull"))
                .Write(BytecodeOp("LoadString", Operand::String("po_feed")))
                .Write(BytecodeOp("GetStatic"))
                .Write(BytecodeOp("Call", Operand::I64(0)))
                .Write(BytecodeOp("Call", Operand::I64(2)))
                .Write(BytecodeOp("Return"))
        )
        .Build();

    rt.AttachFunction("entry", vcaller);
    ort::Value entry = rt.GetStaticObject("entry");

    int ret = -1;

    bench("proxied", [&](int n) {
        for(int i = 0; i < n; i++) {
            ret = rt.Invoke(entry, std::vector<ort::Value>()).ExtractI64();
        }
    });
    printf("%d\n", ret);
}

void test_call() {
    int ret_val = 0;

    ort::Function vcaller = build_call_tester();
    vcaller.EnableOptimization();

    ort::Function native_cb = ort::Function::LoadNative([&ret_val]() {
        ret_val = 42;
        return ort::Value::Null();
    });

    ort::Runtime rt;

    rt.AttachFunction(
        "set_ret",
        native_cb
    );
    rt.AttachFunction(
        "entry",
        vcaller
    );

    ort::Value entry = rt.GetStaticObject("entry");

    bench("invoke", [&](int n) {
        for(int i = 0; i < n; i++) {
            rt.Invoke(entry, std::vector<ort::Value>());
        }
    });

    printf("%d\n", ret_val);
}

int main() {
    test_call();
    test_sum();
    test_proxied();
    test_object_handle();

    return 0;
}
