from rmtest import ModuleTestCase
import redis
import unittest


class SecondaryTestCase(ModuleTestCase('../../build/src/libmodule.so')):

    def testHashIndex(self):
        with self.redis() as r:

            self.assertOk(r.execute_command(
                'idx.create', 'idx', 'type', 'hash', 'schema', 'name', 'string', 'age', 'int32'))

            for i in xrange(100):
                self.assertEquals(1, r.execute_command('idx.into', 'idx', 'HMSET',
                                                       'user%d' % i, 'name', 'name%d' % i, 'age', 10 + i % 30))
            self.assertEqual(100, r.execute_command('idx.card', 'idx'))

            print (r.execute_command('idx.from',
                                     'idx', 'where', "name like 'name%'", 'limit', '0', '10', 'hget', '$', 'name'))

if __name__ == '__main__':

    unittest.main()
