/* Based on
 * RCS (Reverse Computer Screen) by Einar Saukas
 *
 * Note the license on ZX7Pack this extends.
 */

class RCSPack : public ZX7Pack
{
public:
    enum constants
    {
        MAX_SIZE = 6912,
        SECTOR1  = 2048,
        ATTRIB1  = 256
    };
    
    void convert(unsigned char *input_data, unsigned char *output_data) 
    {
        int sector;
        int row;
        int lin;
        int col;
        int i;
    
        i = 0;
    
        /* transform bitmap area */
        for (sector = 0; sector < MAX_SIZE / SECTOR1; sector++) 
        {
            for (col = 0; col < 32; col++) 
            {
                for (row = 0; row < 8; row++) 
                {
                    for (lin = 0; lin < 8; lin++) 
                    {
                        output_data[i++] = input_data[(((((sector<<3)+lin)<<3)+row)<<5)+col];
                    }
                }
            }
        }
    
        /* just copy attributes */
        for (; i < MAX_SIZE; i++) {
            output_data[i] = input_data[i];
        }
    }
    
    unsigned char mRCSFormatData[6912];
    
    virtual void pack(unsigned char *aData, int aLen)        
    {
        convert(aData, mRCSFormatData);
        compress(optimize(mRCSFormatData, aLen), mRCSFormatData, aLen);
    }
};