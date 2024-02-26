import lldb

class Simd128Printer:
    def __init__(self, valobj, internal_dict):
        self.valobj = valobj

    def update(self):
        self.v0 = self.valobj.GetChildAtIndex(0).GetValueAsUnsigned()
        self.v1 = self.valobj.GetChildAtIndex(1).GetValueAsUnsigned()

    def num_children(self):
        return 8

    def get_child_index(self, name):
        return int(name.lstrip('[').rstrip(']'))

    def get_child_at_index(self, index):
        if index == 0:
            return self.valobj.CreateValueFromExpression('u8x16', 'uint16_t x[16] = {' + str(self.v0 & 0xFF) + ',' + str((self.v0 >> 8) & 0xFF) + ',' + str((self.v0 >> 16) & 0xFF) + ',' + str((self.v0 >> 24) & 0xFF) + ',' + str((self.v0 >> 32) & 0xFF) + ',' + str((self.v0 >> 40) & 0xFF) + ',' + str((self.v0 >> 48) & 0xFF) + ',' + str(self.v0 >> 56) + ',' + str(self.v1 & 0xFF) + ',' + str((self.v1 >> 8) & 0xFF) + ',' + str((self.v1 >> 16) & 0xFF) + ',' + str((self.v1 >> 24) & 0xFF) + ',' + str((self.v1 >> 32) & 0xFF) + ',' + str((self.v1 >> 40) & 0xFF) + ',' + str((self.v1 >> 48) & 0xFF) + ',' + str(self.v1 >> 56) + '}; x')
        elif index == 1:
            return self.valobj.CreateValueFromExpression('i8x16', 'int16_t x[16] = {(int8_t)' + str(self.v0 & 0xFF) + ',(int8_t)' + str((self.v0 >> 8) & 0xFF) + ',(int8_t)' + str((self.v0 >> 16) & 0xFF) + ',(int8_t)' + str((self.v0 >> 24) & 0xFF) + ',(int8_t)' + str((self.v0 >> 32) & 0xFF) + ',(int8_t)' + str((self.v0 >> 40) & 0xFF) + ',(int8_t)' + str((self.v0 >> 48) & 0xFF) + ',(int8_t)' + str(self.v0 >> 56) + ',(int8_t)' + str(self.v1 & 0xFF) + ',(int8_t)' + str((self.v1 >> 8) & 0xFF) + ',(int8_t)' + str((self.v1 >> 16) & 0xFF) + ',(int8_t)' + str((self.v1 >> 24) & 0xFF) + ',(int8_t)' + str((self.v1 >> 32) & 0xFF) + ',(int8_t)' + str((self.v1 >> 40) & 0xFF) + ',(int8_t)' + str((self.v1 >> 48) & 0xFF) + ',(int8_t)' + str(self.v1 >> 56) + '}; x')
        elif index == 2:
            return self.valobj.CreateValueFromExpression('u16x8', 'uint16_t x[8] = {' + str(self.v0 & 0xFFFF) + ',' + str((self.v0 >> 16) & 0xFFFF) + ',' + str((self.v0 >> 32) & 0xFFFF) + ',' + str(self.v0 >> 48) + ',' + str(self.v1 & 0xFFFF) + ',' + str((self.v1 >> 16) & 0xFFFF) + ',' + str((self.v1 >> 32) & 0xFFFF) + ',' + str(self.v1 >> 48) + '}; x')
        elif index == 3:
            return self.valobj.CreateValueFromExpression('i16x8', 'int16_t x[8] = {' + str(self.v0 & 0xFFFF) + ',' + str((self.v0 >> 16) & 0xFFFF) + ',' + str((self.v0 >> 32) & 0xFFFF) + ',' + str(self.v0 >> 48) + ',' + str(self.v1 & 0xFFFF) + ',' + str((self.v1 >> 16) & 0xFFFF) + ',' + str((self.v1 >> 32) & 0xFFFF) + ',' + str(self.v1 >> 48) + '}; x')
        elif index == 4:
            return self.valobj.CreateValueFromExpression('u32x4', 'uint32_t x[4] = {' + str(self.v0 & 0xFFFFFFFF) + ',' + str(self.v0 >> 32) + ',' + str(self.v1 & 0xFFFFFFFF) + ',' + str(self.v1 >> 32) + '}; x')
        elif index == 5:
            return self.valobj.CreateValueFromExpression('i32x4', 'int32_t x[4] = {' + str(self.v0 & 0xFFFFFFFF) + ',' + str(self.v0 >> 32) + ',' + str(self.v1 & 0xFFFFFFFF) + ',' + str(self.v1 >> 32) + '}; x')
        elif index == 6:
            return self.valobj.CreateValueFromExpression('u64x2', 'uint64_t x[2] = {' + str(self.v0) + ',' + str(self.v1) + '}; x')
        elif index == 7:
            return self.valobj.CreateValueFromExpression('i64x2', 'int64_t x[2] = {' + str(self.v0) + ',' + str(self.v1) + '}; x')
        else:
            return None

def __lldb_init_module(debugger, dict):
    debugger.HandleCommand('type synthetic add -w simd -l simd.Simd128Printer __m128i')
    debugger.HandleCommand('type category enable simd')
    debugger.HandleCommand('type category disable VectorTypes')
