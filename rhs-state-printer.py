import gdb

class CKRHSPrettyPrinter:
    def __init__(self, val):
        self.val = val

    def to_string(self):
        map_val = self.val['map']
        if map_val == 0:
            return "Empty ck_rhs_t (map is NULL)"
        return self.print_map(map_val)

    def print_map(self, map_val):
        lines = []

        mask = int(map_val['mask'])
        step = int(map_val['step'])
        pl = int(map_val['probe_limit'])
        n_ets = int(map_val['n_entries'])
        capacity = int(map_val['capacity'])
        sz = int(map_val['size'])
        max_entries = int(map_val['max_entries'])
        offset_mask = int(map_val['offset_mask'])

        lines.append("map values:")
        lines.append(f"         mask {mask:>8}        step {step:>8}")
        lines.append(f"  probe_limit {pl:>8} offset mask {offset_mask:>8}")
        lines.append(f"    n_entries {n_ets:>8}    capacity {capacity:>8}")
        lines.append(f"         size {sz:>8} max_entries {max_entries:>8}")

        lines.append("map buckets:")

        mask = map_val['mask']
        entries = map_val['entries']['descs']
        for i in range(mask + 1):
            entry = entries[i]
            probes = entry['probes']
            wanted = entry['wanted']
            probe_bound = int(entry['probe_bound'])
            in_rh = 'T' if entry['in_rh'] else 'F'
            entry_ptr = entry['entry']

            line = (f"  {i}) probes: {probes}, wanted: {wanted}, "
                    f"probe_bound: {probe_bound:d}, in_rh: {in_rh}")

            if entry_ptr != 0:
                line += f", entry: {entry_ptr}"
            else:
                line += f", entry: NULL"

            lines.append(line)

        return "\n".join(lines)

def print_rhs_t(rhs_val):
    printer = CKRHSPrettyPrinter(rhs_val)
    print(printer.to_string())

# Register the command in GDB
class PrintRHSTCommand(gdb.Command):
    """Print the internal state of a ck_rhs_t structure."""

    def __init__(self):
        super(PrintRHSTCommand, self).__init__("print_rhs_t", gdb.COMMAND_DATA)

    def invoke(self, arg, from_tty):
        args = gdb.string_to_argv(arg)

        if len(args) < 1:
            print("Usage: print_rhs_t <ck_rhs_t variable>")
            return

        try:
            rhs_val = gdb.parse_and_eval(args[0])
            print_rhs_t(rhs_val)
        except gdb.error as e:
            print(f"Error: {str(e)}")

PrintRHSTCommand()
