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
        result = "ck_rhs_map entries:\n"
        mask = map_val['mask']
        entries = map_val['entries']['descs']
        for i in range(mask + 1):  # Iterates correctly from 0 to mask
            entry = entries[i]
            probes = entry['probes']
            wanted = entry['wanted']
            probe_bound = entry['probe_bound']
            in_rh = entry['in_rh']
            entry_ptr = entry['entry']
            if entry_ptr != 0:
                entry_key = entry_ptr.cast(gdb.lookup_type('char').pointer()).string()
                result += f"[{i}] probes: {probes}, wanted: {wanted}, probe_bound: {probe_bound}, in_rh: {in_rh}, entry: {entry_key}\n"
            else:
                result += f"[{i}] probes: {probes}, wanted: {wanted}, probe_bound: {probe_bound}, in_rh: {in_rh}, entry: NULL\n"
        return result

def print_rhs_t(rhs_val):
    printer = CKRHSPrettyPrinter(rhs_val)
    print(printer.to_string())

# Register the command in GDB
class PrintRHSTCommand(gdb.Command):
    """Print the internal state of a ck_rhs_t structure."""

    def __init__(self):
        super(PrintRHSTCommand, self).__init__("print_rhs_t", gdb.COMMAND_DATA)

    def invoke(self, arg, from_tty):
        # Parse argument as an expression to a ck_rhs_t instance
        if not arg:
            print("Usage: print_rhs_t <ck_rhs_t variable>")
            return
        try:
            rhs_val = gdb.parse_and_eval(arg)
            print_rhs_t(rhs_val)
        except gdb.error as e:
            print(f"Error: {str(e)}")

PrintRHSTCommand()

