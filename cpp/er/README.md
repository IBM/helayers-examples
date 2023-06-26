# Entity Resolution Example 
`Entity resolution` (ER for short) deals with identifying different records that refer to the same entity, and then handling the identified records as needed. This example demonstrates a specific case of ER, namely `Record-Linkage` (RL), that deals with identifying records in *different* databases which refer to the same entity. Our ER library performs such Record-Linkage in a *privacy-preserving* manner (`PPRL`), in the sense that the sides learn which of their records match similar records in the other DBs, and also the size of the other DBs, but nothing else. So for example, they don't get to learn anything about the non-similar records of the other side, nor the similar but possibly different fields of the identified similar record of the other side.  

Use cases for PPRL are for example a pair of hospitals that wish to learn the common patients that they serve without revealing the identity or any other information about non common patients, and neither any private data of the common patients. Another use case could be two companies that wish to learn what clients they share without revealing anything about the non-common clients.  

The example directory includes the relevant cpp code (er_basic_example.cpp) that uses the public interface of the ER library (liber.a) to perform record-linkage between *two* database tables with about 500,000 records in each database - The first DB belongs to Alice and the 2nd DB belongs to Bob. The tables used by the example are given as a pair of csv files in the *data* subfolder. 

Alice wishes to learn which of her records are similar to records in Bob's table, and Bob is willing to allow Alice to learn this, but not to learn anything else. Alice also does not want Bob to learn anything else beyond the intersection of the tables. 

Most of the records in the two databases in the example describe different entities, but there are about 100 entities that are described by records in both database. The pairs of records that describe these shared entities sometimes have identical fields and sometimes fields with minor typos, different styles, and other types of such minor differences, which are still small enough to warrent the assumption that the similar records in fact describe the same entity. 

## Privacy Preserving Record Linkage Protocol
The example cpp code in er_basic_example.cpp implements the Privacy Preserving Record Linkage (PPRL) Protocol using the 
Helayers ER library API. 
More details can be found in the comments in the cpp file itself. In general, the protocol is similar to the Private-Set-Intersection (PSI) protocol, except that it allowes for similarities rather than insisting on exact equivalence of the reported candidate pairs. 
The similarity of the records is measured in terms of the jaccard similarity index of the two records (see https://en.wikipedia.org/wiki/Jaccard_index). For performance reasons, the Record-Linkage algorithm is statistical so that matched pairs of records probably have a high Jaccard similarity index.

The PPRL protocol proceeds as follows:
1. Alice reads her csv file, performs some record-linkage analysis and encrypts the resulting PPRL information.
2. Alice sends her encrypted PPRL record information to Bob.
3. Bob receives Alice's encrypted PPRL information and adds his own layer of encryption on top of this information (in a Diffie-Hellman like scheme) and sends the doubly encrypted information back to Alice. 

The above 3 steps are now performed starting from Bob's side. That is

4. Bob reads his csv file, performs some record-linkage analysis and encrypts the resulting PPRL information.
5. Bob sends his encrypted PPRL record information to Alice.
6. Alice receives Bob's encrypted PPRL information and adds her own layer of encryption on top of this information (in a Diffie-Hellman like scheme) and sends the doubly encrypted information back to Bob.

7. At this point Alice has both her own records and Bob's records doubly encrypted in a Diffie-Hellman like scheme which makes them comparable. Alice can now compare these doubly encrypted records and report matches as probable identical entities. 

The following variations to the protocol can easily be implemented with minor modifications to the example code 
in er_basic_example.cpp:
- Bob can also compute and learn the intersection - i.e. the records that he probably has in common with Alice.
- Alice (and or Bob) can compute just the size of the intersection. 
 
## Build
Change directory to the examples/cpp/er directory, then execute:

    cmake .
    make

## Run and Validate
Run the ER example 

    ./er_basic_example

This demo would by default compare just the first 1000 records from Alice's and Bob's tables.
You may ask to compare more records (out of the ~500000 records) by using the 
num_records optional flag:

* `num_records` - int, determines the number of records compare from both databases
* `verbose` - sets the verbosity level to high verbosity
* `quiet` - sets the verbosity level to no verbosity

For Example run the following command to compare 5000 records from Alice's DB with 5000 record from Bob's DB with high verbosity:

`./er_basic_example --num_records 5000 --verbose`

To compare ALL the records in both tables, use the value -1. Though note that the full comparison may take some time to complete, depending on the target machine:

`./er_basic_example --num_records -1`

To run the protocol fast but without security for testing and debugging purposes use the following command. Use same flags to set verbosity level and number of records to compare:

`./er_mock`

The output of the example is a list of records from Alice's database that have a duplicate in Bob's database. For example, when running the example on the default 1000 records as described above, the following match report with 100 matching pairs of records is expected, followed by some timing statistics and other meta-data. 

>***
>Comparing 1000 records from each side.  
>Number of bands: 40  
>Size of bands  : 14  
>*** 
>RecordA : Adrian, Cosmo, Adrian.Cosmo, mit.edu, 49049, 1600-1646 Sparksford Dr, Russellville,AR,72802, Suite >?47601, Kicking Horse, OR, USA, 18878, 6687, 715, 301, 5004  
>RecordB : Adrian, Cosmo, Adrian.Cosmo, mit.edu, 49049, 1600-1646 Sparksford Dr, Russellville,AR,72802, Suite 47601, Kicking Horse, OR, USA, 18878, 6687, 715, 301, 5003
>  
>Number of matching bands: 7  
>Jaccard: 0.875  
>***
>RecordA : Elias, Binet, Elias_Binet, aol.com, 30715, 692 Arcadian Way, Charleston,SC,29407, Apartment 56024, >Missisinewa, NC, USA, 97538, 4915, 416, 512, 3724  
>RecordB : Elias, Binet, Elias_Binet, aol.com, 30715, Charleston, 692 Arcadian Way,SC,29407, Apartment 56024, >Missisinewa, NC, USA, 97538, 4915, 416, 512, 3724
>
>Number of matching bands: 1  
>Jaccard: 0.781513
>***
>:
>***
>RecordA : Tiara, Higney, Tiara.Higney, stanford.edu, 49676, 9822 Dorothy Ave, South Gate,CA,90280, Apartment 11276, Fairdealing, OR, USA, 46773, 8924, 608, 889, 9404  
>RecordB : Tiara, Higney, Tiara.Higney, stanford.edu, 49676, 9822 Dorothy Ave, South Gate,CA,90280, Apartment 11276, Fairdealing, OR, USA, 46773, 8924, 608, 889, 9404
>
>Number of matching bands: 40  
>Jaccard: 1
>***
>Timing statistics overview:  
>PPRL: total=0.616 (secs) ( 0.616 X 1)   [CPU: 23.532, 3818.88%]
>--- RecordLinkageManager::applySecretKeyToRecords=0.293 (secs) ( 0.048+-0.055 X 6)   [CPU: 14.758, 5036.66%]
>--- RecordLinkageManager::encryptFieldsForEqualRule=0.016 (secs) ( 0.008+-0.005 X 2)   [CPU: 0.870, 5142.42%]
>--- RecordLinkageManager::encryptFieldsForSimilarRule=0.165 (secs) ( 0.041+-0.022 X 4)   [CPU: 7.712, 4668.46%]
>--- RecordLinkageManager::initRecordsFromFile=0.031 (secs) ( 0.015+-0.000 X 2)   [CPU: 0.031, 99.90%]
>--- RecordLinkageManager::matchRecordsByEqualRule=0.000 (secs) ( 0.000+-0.000 X 2)   [CPU: 0.018, 2835.90%]
>--- RecordLinkageManager::matchRecordsBySimilarRule=0.104 (secs) ( 0.026+-0.017 X 4)   [CPU: 0.104, 99.98%]
>--- RecordLinkageManager::reportMatchedRecords=0.000 (secs) ( 0.000 X 1)   [CPU: 0.000, 100.00%]
>--- RecordLinkageManager::reportMatchedRecordsAlongWithOtherSideRecords=0.001 (secs) ( 0.001 X 1)   [CPU: 0.001, 100.00%] 
>***
>Number of records analyzed from Alice's side: 1000  
>Number of records analyzed from Bob's side  : 1000  
>Number of matching similar records          : 100
>***

As can be seen above, for every record of Alice that has a duplicate in Bob's table, the report includes the number of shared "bands" for the pair. This is a technical term
that relates to the MinHash algorithm that is used to compare the records, but in general at least one shared band is required and sufficient to warrent a report of a probable candidate pair of records. More shared bands indicate a higher probability that the pair of records indeed describe the same entity. Thus the 1st record reported above (with 7 bands for Adrian Cosmo) probably has a matching record in Bob's DB, the 2nd record (with 1 band for Elias Binet) also probably has a match but somewhat less similar, whereas the last record reported above (with 40 bands for Janel Rishel) definitely has an identical record in Bob's DB. 

Note that the report for a match also gives the matching record in Bob's DB and the Jaccard similarity measure, but this is given only for demonstration purposes and is possible only because this demo has access both to Alice and Bob's DBs. In a secure Record-Linkage application the number of bands measure is still available to Alice, but not the precise matching record from Bob's DB nor the Jaccard index.
