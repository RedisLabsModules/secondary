from rmtest import ModuleTestCase
import redis
import unittest
from redis.exceptions import RedisError


class SecondaryTestCase(ModuleTestCase('../../build/src/libmodule.so')):

    def execFromWhere(self, r, idx, where, cmd):

        return r.execute_command("idx.from", "idx", "where", where, *cmd.split(" "))

    def execInto(self, r, idx, cmd, where=None):
        if where:
            return r.execute_command("idx.into", "idx", "where", where, *cmd.split(" "))
        else:
            return r.execute_command("idx.into", "idx", *cmd.split(" "))

    def testHashIndex(self):

        with self.redis() as r:

            self.assertOk(r.execute_command(
                'idx.create', 'idx', 'type', 'hash', 'schema', 'name', 'string', 'age', 'int32'))

            for i in xrange(100):
                self.assertEquals(1, r.execute_command('idx.into', 'idx', 'HMSET',
                                                       'user%d' % i, 'name', 'name%d' % i, 'age', 10 + i % 30))
            self.assertEqual(100, r.execute_command('idx.card', 'idx'))

            res = self.execFromWhere(
                r, "idx", "name like 'name1%'", 'hget $ name')

            self.assertEqual(res, ['user1', 'name1', 'user10', 'name10', 'user11', 'name11', 'user12', 'name12', 'user13', 'name13',
                                   'user14', 'name14', 'user15', 'name15', 'user16', 'name16', 'user17', 'name17', 'user18', 'name18', 'user19', 'name19'])

            # test equality
            self.assertEqual(['user10', 'name10'], self.execFromWhere(
                r, "idx", "name = 'name10'", 'hget $ name'))

            # test less than
            self.assertEqual(['user0', 'name0', 'user1', 'name1'],
                             self.execFromWhere(r, "idx", "name < 'name10'", 'hget $ name'))
            self.assertEqual(['user0', 'name0', 'user1', 'name1', 'user10', 'name10'],
                             self.execFromWhere(r, "idx", "name <= 'name10'", 'hget $ name'))

            # test greater than / equals
            self.assertEqual(['user96', 'name96', 'user97', 'name97', 'user98', 'name98', 'user99', 'name99'],
                             self.execFromWhere(r, "idx", "name > 'name95'", 'hget $ name'))
            self.assertEqual(['user95', 'name95', 'user96', 'name96', 'user97', 'name97', 'user98', 'name98', 'user99', 'name99'],
                             self.execFromWhere(r, "idx", "name >= 'name95'", 'hget $ name'))

            # test IN
            self.assertEqual(['user95', 'name95', 'user10', 'name10', 'user35', 'name35'],
                             self.execFromWhere(r, "idx", "name IN('name95', 'name10', 'name35')", 'hget $ name'))

            # test NULL
            self.assertEqual(1L, self.execInto(r, 'idx', 'HDEL user35 name'))
            self.assertEqual(['user35', None],
                             self.execFromWhere(r, "idx", "name IS NULL", 'hget $ name'))

            # test multiple predicates
            self.assertTrue(['user11', ['name11', '21'], 'user12', ['name12', '22'], 'user13', ['name13', '23'], 'user14', ['name14', '24'], 'user41', ['name41', '21'], 'user42', ['name42', '22'], 'user43', ['name43', '23'], 'user44', ['name44', '24']],
                            self.execFromWhere(r, "idx", "name < 'name50' AND (age > 20 AND age < 25)", 'hmget $ name age'))

            # test deletion
            self.assertEqual(['user12', 'name12'],
                             self.execFromWhere(r, "idx", "name = 'name12'", 'hget $ name'))
            self.assertEqual(1, self.execInto(r, 'idx', 'DEL user12'))
            self.assertEqual([],
                             self.execFromWhere(r, "idx", "name = 'name12'", 'hget $ name'))
            self.assertEqual(99, r.execute_command('idx.card', 'idx'))

    def testRawIndex(self):

        with self.redis() as r:

            self.assertOk(r.execute_command(
                'idx.create', 'idx', 'schema', 'string',  'int32'))

            for i in range(100):
                self.assertOk(r.execute_command('idx.insert', 'idx', 'id%d' %
                                                i, 'str%d' % i, i))

            self.assertEqual(100, r.execute_command('idx.card', 'idx'))

            self.assertEqual(['id10'],  r.execute_command(
                'idx.select', 'idx', 'WHERE', "$1 = 'str10' AND $2 = 10"))

            # Test deletion
            self.assertEqual(['id1', 'id2', 'id30'],  r.execute_command(
                'idx.select', 'idx', 'WHERE', "$1 IN('str1', 'str2', 'str30')"))

            self.assertOk(r.execute_command(
                'idx.del', 'idx', 'id1', 'id2', 'id30'))

            self.assertEqual([],  r.execute_command(
                'idx.select', 'idx', 'WHERE', "$1 IN('str1', 'str2', 'str30')"))

            self.assertEqual(97, r.execute_command('idx.card', 'idx'))

    def testUniqueIndex(self):

        with self.redis() as r:
            self.assertOk(r.execute_command(
                'idx.create', 'idx', 'unique', 'schema', 'string'))

            self.assertOk(r.execute_command('idx.insert', 'idx', 'id1', 'foo'))

            self.assertRaises(RedisError, r.execute_command,
                              'idx.insert', 'idx', 'id2', 'foo')
            self.assertOk(r.execute_command('idx.insert', 'idx', 'id1', 'foo'))
            self.assertEqual(1, r.execute_command('idx.card', 'idx'))

            self.assertOk(r.execute_command('idx.insert', 'idx', 'id2', 'bar'))
            self.assertOk(r.execute_command('idx.del', 'idx', 'id1'))
            self.assertOk(r.execute_command('idx.insert', 'idx', 'id3', 'foo'))

            self.assertEqual(2, r.execute_command('idx.card', 'idx'))

    def testTimeFunctions(self):
        pass

if __name__ == '__main__':

    unittest.main()
