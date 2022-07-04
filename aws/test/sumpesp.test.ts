import { expect as expectCDK, matchTemplate, MatchStyle } from '@aws-cdk/assert';
import * as cdk from '@aws-cdk/core';
import * as esp from '../lib/sumpesp-stack';

test('Empty Stack', () => {
    const app = new cdk.App();
    // WHEN
    const stack = new esp.SumpESPStack(app, 'MyTestStack');
    // THEN
    expectCDK(stack).to(matchTemplate({
      "Resources": {}
    }, MatchStyle.EXACT))
});
