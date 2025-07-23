import dis

def simplify_bytecode(bytecode: dis.Bytecode):
    """
    Returns a list of tuples, which specify the start and end indices of
    instructions that are considered redundant.
    """

    ignore_regions: list[tuple[int, int]] = []
    fn = bytecode.codeobj

    body = [*bytecode]

    # Remove writes to __static_attributes__
    for idx in range(len(body) - 1):
        i1 = body[idx]
        i2 = body[idx + 1]

        qualifies = (
            i1.opname == "LOAD_CONST"
            and i1.arg is not None
            and type(fn.co_consts[i1.arg]) is tuple
        ) and (
            i2.opname == "STORE_NAME"
            and i2.arg is not None
            and fn.co_names[i2.arg] == "__static_attributes__"
        )

        if qualifies:
            ignore_regions.append((idx, idx + 1))

    # Remove annotations
    for idx in range(len(body)):
        if not (body[idx].opname == "SET_FUNCTION_ATTRIBUTE" and body[idx].arg == 0x04):
            continue

        assert body[idx - 1].opname == "MAKE_FUNCTION"
        assert body[idx - 2].opname == "LOAD_CONST"

        build_tuple_idx = idx - 3
        if not body[build_tuple_idx].opname == "BUILD_TUPLE":
            continue

        start = idx - 4
        assert start >= 0

        while body[start].opname in ["LOAD_CONST", "LOAD_NAME"] and start > 0:
            start -= 1

        # Ignore the creation of the attribute tuple...
        ignore_regions.append((start, build_tuple_idx))

        # ...then leave the actual creation of the function object alone...

        # ...and finally, remove the `SET_FUNCTION_ATTRIBUTE` instruction after the function
        # object is created.
        ignore_regions.append((idx, idx))

    return ignore_regions
