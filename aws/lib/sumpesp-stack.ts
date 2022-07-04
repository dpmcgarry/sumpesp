import * as cdk from '@aws-cdk/core';
import * as s3 from '@aws-cdk/aws-s3';


export class SumpESPStack extends cdk.Stack {
  constructor(scope: cdk.Construct, id: string, props?: cdk.StackProps) {
    super(scope, id, props);

    const dataBucket = new s3.Bucket(this, "sumpesp-data",{
      blockPublicAccess: s3.BlockPublicAccess.BLOCK_ALL,
      encryption: s3.BucketEncryption.S3_MANAGED
    });

    

  }
}
