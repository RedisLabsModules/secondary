# disposable-redis

Disposable Redis for your Unittests

## Installation

`pip install disposable-redis`

## Usage

```python
import unittest
from disposableredis import DisposableRedis

class RedisTestCase(unittest.TestCase):
    def test_redis_set_get(self):
        with DisposableRedis() as client:
            client.set('key', 'value')
            self.assertEqual(client.get('key'), 'value')
```
