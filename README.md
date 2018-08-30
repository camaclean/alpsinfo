# alpsinfo
Query ALPS info with Python on Cray supercomputers

Usage:

```python
import alpsinfo, pprint
pprint.pprint(alpsinfo.info())
```

Example output:

```
{'apid': 672983,
'cmds': [{'accel': 'None',
          'cpusPerCU': 2,
          'depth': 16,
          'fixedPerNode': 1,
          'nodeCnt': 1,
          'nodeSegCnt': 0,
          'pesPerSeg': 0,
          'segBits': 0,
          'width': 1}]}
```
