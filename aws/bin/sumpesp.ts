#!/usr/bin/env node
import 'source-map-support/register';
import * as cdk from '@aws-cdk/core';
import { SumpESPStack } from '../lib/sumpesp-stack';

const app = new cdk.App();
new SumpESPStack(app, 'AwsStack');
